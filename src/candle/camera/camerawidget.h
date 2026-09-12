#pragma once

#include <QImage>
#include <QVideoFrame>
#include <QWidget>

class QCamera;
class QCameraDevice;
class QCheckBox;
class QComboBox;
class QMediaCaptureSession;
class QVideoSink;

// Native Qt 6 live-preview widget. It intentionally does no vision inference:
// external vision services use Candle's localhost automation API.
class CameraWidget final : public QWidget {
    Q_OBJECT
public:
    explicit CameraWidget(QWidget *parent = nullptr);
    ~CameraWidget() override;
    bool hasFrame() const { return !m_frame.isNull(); }
    QStringList deviceNames() const;
    QByteArray currentDeviceId() const;
    void setCurrentDeviceId(const QByteArray &id);
    bool mirrored() const;
    void setMirrored(bool value);
    void refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void refreshDevices();
    void selectDevice(int index);
    void processFrame(const QVideoFrame &frame);

private:
    void startCamera(const QCameraDevice &device);
    void stopCamera();

    QComboBox *m_devices = nullptr;
    QCheckBox *m_mirror = nullptr;
    QCamera *m_camera = nullptr;
    QMediaCaptureSession *m_capture = nullptr;
    QVideoSink *m_sink = nullptr;
    QImage m_frame;
};
