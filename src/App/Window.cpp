#include "Window.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QScreen>
#include <QVBoxLayout>

#include <array>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

namespace
{
struct vert {
	GLfloat position[2];
};

constexpr std::array<vert, 4u> vertices = {
	vert{
		.position = {-1.0f, -1.0f},
	},
	vert{
		.position = {1.0f, 1.0f},
	},
	vert{
		.position = {-1.0f, 1.0f},
	},
	vert{
		.position = {1.0f, -1.0f},
	},
};
constexpr std::array<GLuint, 6u> indices = {0, 1, 2, 0, 3, 1};

}// namespace

void Window::setupUI()
{
	auto * mainLayout = new QVBoxLayout();

	auto * fractalGroup = new QGroupBox("Параметры фрактала");
	auto * fractalLayout = new QFormLayout();

	iterationsSlider_ = new QSlider(Qt::Horizontal);
	iterationsSlider_->setRange(10, 2000);
	iterationsSlider_->setValue(info.maxIterations);
	iterationsSlider_->setTickInterval(50);
	iterationsSlider_->setTickPosition(QSlider::TicksBelow);
	iterationsLabel_ = new QLabel(QString::number(info.maxIterations));
	fractalLayout->addRow("Итерации:", iterationsSlider_);
	fractalLayout->addRow("", iterationsLabel_);

	fractalGroup->setLayout(fractalLayout);
	fractalGroup->setMaximumWidth(250);

	auto * fpsGroup = new QGroupBox("Производительность");
	auto * fpsLayout = new QVBoxLayout();
	auto * fpsLabel = new QLabel("FPS: 0", this);
	fpsLabel->setStyleSheet("QLabel { color : white; font-weight: bold; }");
	fpsLayout->addWidget(fpsLabel);
	fpsGroup->setLayout(fpsLayout);
	fpsGroup->setMaximumWidth(250);

	connect(iterationsSlider_, &QSlider::valueChanged, this, &Window::onSliderChanged);

	connect(this, &Window::updateUI, [=] {
		fpsLabel->setText(QString("FPS: %1").arg(ui_.fps));
	});

	mainLayout->addWidget(fractalGroup);
	mainLayout->addWidget(fpsGroup);
	mainLayout->addStretch(1);

	setLayout(mainLayout);
}

void Window::onSliderChanged()
{
	info.maxIterations = iterationsSlider_->value();
	iterationsLabel_->setText(QString::number(info.maxIterations));

	update();
}

Window::Window() noexcept
{
	info.resolution = {640.0f, 480.0f};
	info.offset = {0.0f, 0.0f};
	info.zoom = 1.0f;
	info.maxIterations = 100;

	setupUI();
	timer_.start();
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
		program_.reset();
	}
}

void Window::onInit()
{
	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/vertex.glsl");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/fragment.glsl");
	program_->link();

	// Create VAO object
	vao_.create();
	vao_.bind();

	// Create VBO
	vbo_.create();
	vbo_.bind();
	vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	vbo_.allocate(vertices.data(), static_cast<int>(sizeof(vertices)));

	// Create IBO
	ibo_.create();
	ibo_.bind();
	ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	ibo_.allocate(indices.data(), static_cast<int>(sizeof(indices)));

	// Bind attributes
	program_->bind();

	program_->enableAttributeArray(0);
	program_->setAttributeBuffer(0, GL_FLOAT, static_cast<int>(offsetof(vert, position)), 2, static_cast<int>(sizeof(vert)));

	resolutionUniform_ = program_->uniformLocation("resolution");
	offsetUniform_ = program_->uniformLocation("offset");
	zoomUniform_ = program_->uniformLocation("zoom");
	timeUniform_ = program_->uniformLocation("time");
	maxIterationsUniform_ = program_->uniformLocation("maxIterations");

	// Release all
	program_->release();

	vao_.release();

	ibo_.release();
	vbo_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::onRender()
{
	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	updateUniform();

	// Draw
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

	// Release VAO and shader program
	vao_.release();
	program_->release();

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	// Configure viewport
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	info.resolution = {static_cast<GLfloat>(width), static_cast<GLfloat>(height)};

	// Update the uniform if program is created
	if (program_ && program_->isLinked())
	{
		const auto guard = bindContext();
		program_->bind();
		updateUniform();
		program_->release();
	}
}

void Window::updateUniform()
{
	if (program_ && program_->isLinked())
	{
		// Upload individual uniform components
		if (resolutionUniform_ != -1)
		{
			program_->setUniformValue(resolutionUniform_, info.resolution);
		}

		if (offsetUniform_ != -1)
		{
			program_->setUniformValue(offsetUniform_, info.offset);
		}

		if (zoomUniform_ != -1)
		{
			program_->setUniformValue(zoomUniform_, info.zoom);
		}

		if (timeUniform_ != -1)
		{
			GLfloat time = static_cast<float>(QDateTime::currentMSecsSinceEpoch() % (31415 * 2));
			program_->setUniformValue(timeUniform_, time);
		}

		if (maxIterationsUniform_ != -1)
		{
			program_->setUniformValue(maxIterationsUniform_, info.maxIterations);
		}
	}
}

void Window::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		lastMousePos_ = event->pos();
		isPanning_ = true;
	}
}

void Window::mouseReleaseEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton)
	{
		isPanning_ = false;
	}
}

void Window::mouseMoveEvent(QMouseEvent * event)
{
	if (isPanning_ && (event->buttons() & Qt::LeftButton))
	{
		QPointF delta = event->pos() - lastMousePos_;
		delta.ry() = -delta.y();
		lastMousePos_ = event->pos();

		// Calculate panning in Mandelbrot space
		// Convert pixel delta to Mandelbrot space delta
		// Adjust for aspect ratio and zoom
		QPointF normalized = 2.0f * delta / (info.resolution.y() * info.zoom);
		info.offset -= normalized;

		update();// Request redraw
	}
}

void Window::wheelEvent(QWheelEvent * event)
{
	// Get mouse position relative to window center
	QPointF mousePos = event->position();

	// Convert mouse position to normalized device coordinates [-1, 1]
	float ndcX = (2.0f * mousePos.x() / info.resolution.x()) - 1.0f;
	float ndcY = 1.0f - (2.0f * mousePos.y() / info.resolution.y());// Flip Y

	float aspectRatio = info.resolution.x() / info.resolution.y();
	ndcX *= aspectRatio;

	QPointF ndc = {ndcX, ndcY};

	// Get current position in Mandelbrot space at cursor
	QPointF currentCursorPos = info.offset + ndc / info.zoom;

	// Apply zoom
	float zoomFactor = 1.25f;// Slower, more precise zoom
	QPointF zoomDelta = event->angleDelta();

	if (zoomDelta.y() > 0)
	{
		// Zoom in
		info.zoom *= zoomFactor;
	}
	else if (zoomDelta.y() < 0)
	{
		// Zoom out
		info.zoom /= zoomFactor;
	}

	// Calculate new offset to keep cursor position fixed
	QPointF newCursorPos = info.offset + ndc / info.zoom;
	QPointF delta = (currentCursorPos - newCursorPos);
	info.offset += delta;

	update();// Request redraw
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{std::move(callback)}
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}};
}
