import bb.cascades 1.4
import bb.system 1.2

Sheet {
    id: imagePreviewSheet

    property real imageScale: 1.0
    property real pinchStartScale: 1.0
    property real imageOffsetX: 0.0
    property real imageOffsetY: 0.0
    property real dragStartX: 0.0
    property real dragStartY: 0.0
    property real dragStartOffsetX: 0.0
    property real dragStartOffsetY: 0.0
    property real fittedImageWidth: 0.0
    property real fittedImageHeight: 0.0

    attachedObjects: [
        SystemToast {
            id: imagePreviewToast
        }
    ]

    onClosed: {
        imagePreview.close();
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Image Viewer")
            dismissAction: ActionItem {
                imageSource: "asset:///images/icons/accent/caret-left.png"
                onTriggered: imagePreviewSheet.close()
            }

            acceptAction: ActionItem {
                imageSource: "asset:///images/icons/accent/ic_save_image.png"
                enabled: !imagePreview.loading && imagePreview.imageSource !== ""

                onTriggered: {
                    imagePreview.saveImage();
                }
            }
        }

        Container {
            id: previewArea
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.Black

            layout: DockLayout {}

            Container {
                id: imageContainer
                visible: imagePreview.imageSource !== ""
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: imagePreviewSheet.fittedImageWidth
                preferredHeight: imagePreviewSheet.fittedImageHeight
                translationX: imagePreviewSheet.imageOffsetX
                translationY: imagePreviewSheet.imageOffsetY
                scaleX: imagePreviewSheet.imageScale
                scaleY: imagePreviewSheet.imageScale

                ImageView {
                    id: fullImage
                    imageSource: imagePreview.imageSource
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    scalingMethod: ScalingMethod.Fill
                }
            }

            onTouch: {
                if (imagePreviewSheet.imageScale <= 1.0) {
                    return;
                }

                if (event.touchType == TouchType.Down) {
                    imagePreviewSheet.dragStartX = event.windowX;
                    imagePreviewSheet.dragStartY = event.windowY;
                    imagePreviewSheet.dragStartOffsetX = imagePreviewSheet.imageOffsetX;
                    imagePreviewSheet.dragStartOffsetY = imagePreviewSheet.imageOffsetY;
                } else if (event.touchType == TouchType.Move) {
                    imagePreviewSheet.imageOffsetX = imagePreviewSheet.dragStartOffsetX + event.windowX - imagePreviewSheet.dragStartX;
                    imagePreviewSheet.imageOffsetY = imagePreviewSheet.dragStartOffsetY + event.windowY - imagePreviewSheet.dragStartY;
                    imagePreviewSheet.clampImageOffset();
                } else if (event.touchType == TouchType.Up || event.touchType == TouchType.Cancel) {
                    imagePreviewSheet.fitIfTooSmall();
                }
            }

            Container {
                visible: imagePreview.loading || imagePreview.failed
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: ui.du(48.0)
                leftPadding: ui.du(2.0)
                rightPadding: ui.du(2.0)
                topPadding: ui.du(2.0)
                bottomPadding: ui.du(2.0)

                layout: StackLayout {}

                ProgressIndicator {
                    visible: imagePreview.loading
                    fromValue: 0
                    toValue: 100
                    value: imagePreview.progress
                    state: imagePreview.progress > 0 ? ProgressIndicatorState.Progress : ProgressIndicatorState.Indeterminate
                    horizontalAlignment: HorizontalAlignment.Fill
                }

                Label {
                    text: imagePreview.statusText
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.fontSize: FontSize.XSmall
                    textStyle.color: Color.White
                }
            }

            gestureHandlers: [
                PinchHandler {
                    onPinchStarted: {
                        imagePreviewSheet.updateFittedImageSize();
                        imagePreviewSheet.pinchStartScale = imagePreviewSheet.imageScale;
                    }

                    onPinchUpdated: {
                        var nextScale = imagePreviewSheet.pinchStartScale * event.pinchRatio;
                        if (nextScale < 0.5) {
                            nextScale = 0.5;
                        }
                        if (nextScale > 6.0) {
                            nextScale = 6.0;
                        }
                        imagePreviewSheet.imageScale = nextScale;
                        imagePreviewSheet.clampImageOffset();
                    }

                    onPinchEnded: {
                        imagePreviewSheet.fitIfTooSmall();
                    }
                },
                DoubleTapHandler {
                    onDoubleTapped: {
                        if (imagePreviewSheet.imageScale > 1.0) {
                            imagePreviewSheet.resetImageTransform();
                        } else {
                            imagePreviewSheet.imageScale = 2.0;
                        }
                    }
                }
            ]
        }
    }

    function openUrl(url, imageWidth, imageHeight) {
        resetImageTransform();
        imagePreview.open(url, imageWidth, imageHeight);
        open();
    }

    function resetImageTransform() {
        imageScale = 1.0;
        pinchStartScale = 1.0;
        imageOffsetX = 0.0;
        imageOffsetY = 0.0;
        dragStartX = 0.0;
        dragStartY = 0.0;
        dragStartOffsetX = 0.0;
        dragStartOffsetY = 0.0;
        updateFittedImageSize();
    }

    function fitIfTooSmall() {
        if (imageScale < 1.0) {
            resetImageTransform();
        }
        if (imageScale <= 1.0) {
            imageOffsetX = 0.0;
            imageOffsetY = 0.0;
        } else {
            clampImageOffset();
        }
    }

    function clampImageOffset() {
        updateFittedImageSize();

        if (imageScale <= 1.0) {
            imageOffsetX = 0.0;
            imageOffsetY = 0.0;
            return;
        }

        var areaWidth = previewWidth();
        var areaHeight = previewHeight();
        var scaledWidth = fittedImageWidth * imageScale;
        var scaledHeight = fittedImageHeight * imageScale;
        var maxOffsetX = (scaledWidth - areaWidth) / 2.0;
        var maxOffsetY = (scaledHeight - areaHeight) / 2.0;

        if (maxOffsetX < 0.0) {
            maxOffsetX = 0.0;
        }
        if (maxOffsetY < 0.0) {
            maxOffsetY = 0.0;
        }

        if (imageOffsetX > maxOffsetX) {
            imageOffsetX = maxOffsetX;
        } else if (imageOffsetX < -maxOffsetX) {
            imageOffsetX = -maxOffsetX;
        }

        if (imageOffsetY > maxOffsetY) {
            imageOffsetY = maxOffsetY;
        } else if (imageOffsetY < -maxOffsetY) {
            imageOffsetY = -maxOffsetY;
        }
    }

    function updateFittedImageSize() {
        var areaWidth = previewWidth();
        var areaHeight = previewHeight();
        var sourceWidth = imagePreview.imageWidth;
        var sourceHeight = imagePreview.imageHeight;

        if (areaWidth <= 0) {
            areaWidth = ui.du(100.0);
        }
        if (areaHeight <= 0) {
            areaHeight = ui.du(100.0);
        }

        if (sourceWidth <= 0 || sourceHeight <= 0) {
            fittedImageWidth = areaWidth;
            fittedImageHeight = areaHeight;
            return;
        }

        var imageRatio = sourceWidth / sourceHeight;
        var areaRatio = areaWidth / areaHeight;
        if (imageRatio > areaRatio) {
            fittedImageWidth = areaWidth;
            fittedImageHeight = areaWidth / imageRatio;
        } else {
            fittedImageHeight = areaHeight;
            fittedImageWidth = areaHeight * imageRatio;
        }
    }

    function previewWidth() {
        if (previewArea.layoutFrame !== undefined && previewArea.layoutFrame.width !== undefined) {
            return previewArea.layoutFrame.width;
        }
        if (previewArea.width !== undefined) {
            return previewArea.width;
        }
        return 0.0;
    }

    function previewHeight() {
        if (previewArea.layoutFrame !== undefined && previewArea.layoutFrame.height !== undefined) {
            return previewArea.layoutFrame.height;
        }
        if (previewArea.height !== undefined) {
            return previewArea.height;
        }
        return 0.0;
    }

    onCreationCompleted: {
        imagePreview.saveSucceeded.connect(function (path) {
            imagePreviewToast.body = qsTr("Saved image");
            imagePreviewToast.show();
        });
        imagePreview.saveFailed.connect(function (message) {
            imagePreviewToast.body = message;
            imagePreviewToast.show();
        });
    }
}