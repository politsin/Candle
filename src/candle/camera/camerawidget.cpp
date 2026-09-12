#include "camerawidget.h"

#include <QCamera>
#include <QCameraDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QVideoSink>

CameraWidget::CameraWidget(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    auto *toolbar = new QHBoxLayout();
    m_devices = new QComboBox(this);
    auto *refresh = new QPushButton(tr("Refresh"), this);
    m_mirror = new QCheckBox(tr("Mirror"), this);
    toolbar->addWidget(m_devices, 1);
    toolbar->addWidget(refresh);
    toolbar->addWidget(m_mirror);
    layout->addLayout(toolbar);
    layout->addStretch(1);
    setMinimumSize(320, 240);
    setStyleSheet("CameraWidget { background: black; }");
    connect(refresh, &QPushButton::clicked, this, &CameraWidget::refreshDevices);
    connect(m_devices, qOverload<int>(&QComboBox::currentIndexChanged), this, &CameraWidget::selectDevice);
    connect(m_mirror, &QCheckBox::toggled, this, qOverload<>(&CameraWidget::update));
    refreshDevices();
}

CameraWidget::~CameraWidget() { stopCamera(); }

QStringList CameraWidget::deviceNames() const {
    QStringList result;
    for (const auto &device : QMediaDevices::videoInputs()) result << device.description();
    return result;
}

QByteArray CameraWidget::currentDeviceId() const { return m_devices->currentData().toByteArray(); }

void CameraWidget::setCurrentDeviceId(const QByteArray &id) {
    const int index = m_devices->findData(id);
    if (index >= 0) m_devices->setCurrentIndex(index);
}

bool CameraWidget::mirrored() const { return m_mirror->isChecked(); }
void CameraWidget::setMirrored(const bool value) { m_mirror->setChecked(value); }
void CameraWidget::refresh() { refreshDevices(); }

void CameraWidget::refreshDevices() {
    const QByteArray current = m_devices->currentData().toByteArray();
    QSignalBlocker blocker(m_devices);
    m_devices->clear();
    const auto devices = QMediaDevices::videoInputs();
    for (const auto &device : devices) m_devices->addItem(device.description(), device.id());
    int index = m_devices->findData(current);
    if (index < 0 && m_devices->count()) index = 0;
    m_devices->setCurrentIndex(index);
    blocker.unblock();
    selectDevice(index);
}

void CameraWidget::selectDevice(const int index) {
    const auto devices = QMediaDevices::videoInputs();
    if (index < 0 || index >= devices.size()) { stopCamera(); return; }
    startCamera(devices.at(index));
}

void CameraWidget::startCamera(const QCameraDevice &device) {
    stopCamera();
    m_camera = new QCamera(device, this);
    m_capture = new QMediaCaptureSession(this);
    m_sink = new QVideoSink(this);
    m_capture->setCamera(m_camera);
    m_capture->setVideoSink(m_sink);
    connect(m_sink, &QVideoSink::videoFrameChanged, this, &CameraWidget::processFrame);
    m_camera->start();
}

void CameraWidget::stopCamera() {
    if (m_camera) m_camera->stop();
    delete m_capture; m_capture = nullptr;
    delete m_sink; m_sink = nullptr;
    delete m_camera; m_camera = nullptr;
    m_frame = QImage();
    update();
}

void CameraWidget::processFrame(const QVideoFrame &frame) {
    const QImage image = frame.toImage();
    if (!image.isNull()) { m_frame = image; update(); }
}

void CameraWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    const QRect area = contentsRect().adjusted(4, 40, -4, -4);
    p.fillRect(area, Qt::black);
    if (!m_frame.isNull()) {
        const QImage image = m_mirror->isChecked() ? m_frame.mirrored(true, false) : m_frame;
        const QPixmap pixmap = QPixmap::fromImage(image).scaled(area.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(area.x() + (area.width() - pixmap.width()) / 2, area.y() + (area.height() - pixmap.height()) / 2, pixmap);
    } else {
        p.setPen(Qt::lightGray); p.drawText(area, Qt::AlignCenter, tr("No camera frame"));
    }
    const QPoint center = area.center();
    p.setPen(QPen(Qt::red, 1));
    p.drawLine(area.left(), center.y(), area.right(), center.y());
    p.drawLine(center.x(), area.top(), center.x(), area.bottom());
}
