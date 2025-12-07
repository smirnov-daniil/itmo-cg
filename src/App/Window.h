#pragma once

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QSlider>
#include <QVBoxLayout>

#include <functional>
#include <memory>

struct Info {
	QPointF resolution;
	QPointF offset;
	GLfloat zoom;
	GLint maxIterations = 100;
};

class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public:// fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent * event) override;

private:
	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

	void updateUniform();
	void setupUI();
	void onSliderChanged();

signals:
	void updateUI();

private:
	QPoint lastMousePos_;
	bool isPanning_ = false;

	GLint
		resolutionUniform_ = -1,
		offsetUniform_ = -1,
		cursorUniform_ = -1,
		zoomUniform_ = -1,
		timeUniform_ = -1,
		maxIterationsUniform_ = -1;

	Info info;

	QSlider * iterationsSlider_;
	QLabel * iterationsLabel_;

	QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
	QOpenGLBuffer ibo_{QOpenGLBuffer::Type::IndexBuffer};
	QOpenGLVertexArrayObject vao_;

	std::unique_ptr<QOpenGLShaderProgram> program_;

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;
};
