import bb.cascades 1.4
import bb.system 1.2

Sheet {
    id: imagePreviewSheet
    peekEnabled: false

    property real imageScale: 1.0
    property real pinchStartScale: 1.0
    property real imageOffsetX: 0.0
    property real imageOffsetY: 0.0
    property real minImageScale: 0.5
    property real maxImageScale: 6.0
    property bool isPinching: false
    property real dragStartX: 0.0
    property real dragStartY: 0.0
    property real dragStartOffsetX: 0.0
    property real dragStartOffsetY: 0.0
    property bool dragMoved: false
    property real pinchStartOffsetX: 0.0
    property real pinchStartOffsetY: 0.0
    property real pinchPivotX: 0.0
    property real pinchPivotY: 0.0
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
                    scalingMethod: ScalingMethod.AspectFit
                }
            }

            onTouch: {
                if (imagePreviewSheet.isPinching || !imagePreviewSheet.canPanImage()) {
                    return;
                }

                if (event.touchType == TouchType.Down) {
                    imagePreviewSheet.dragStartX = event.windowX;
                    imagePreviewSheet.dragStartY = event.windowY;
                    imagePreviewSheet.dragStartOffsetX = imagePreviewSheet.imageOffsetX;
                    imagePreviewSheet.dragStartOffsetY = imagePreviewSheet.imageOffsetY;
                    imagePreviewSheet.dragMoved = false;
                } else if (event.touchType == TouchType.Move) {
                    var deltaX = event.windowX - imagePreviewSheet.dragStartX;
                    var deltaY = event.windowY - imagePreviewSheet.dragStartY;
                    if (Math.abs(deltaX) > ui.du(0.5) || Math.abs(deltaY) > ui.du(0.5)) {
                        imagePreviewSheet.dragMoved = true;
                    }
                    imagePreviewSheet.imageOffsetX = imagePreviewSheet.rubberBandOffset(imagePreviewSheet.dragStartOffsetX + deltaX, imagePreviewSheet.maxOffsetX());
                    imagePreviewSheet.imageOffsetY = imagePreviewSheet.rubberBandOffset(imagePreviewSheet.dragStartOffsetY + deltaY, imagePreviewSheet.maxOffsetY());
                } else if (event.touchType == TouchType.Up || event.touchType == TouchType.Cancel) {
                    imagePreviewSheet.settleImageTransform();
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
                        imagePreviewSheet.pinchStartOffsetX = imagePreviewSheet.imageOffsetX;
                        imagePreviewSheet.pinchStartOffsetY = imagePreviewSheet.imageOffsetY;
                        imagePreviewSheet.pinchPivotX = imagePreviewSheet.eventX(event);
                        imagePreviewSheet.pinchPivotY = imagePreviewSheet.eventY(event);
                        imagePreviewSheet.isPinching = true;
                    }

                    onPinchUpdated: {
                        imagePreviewSheet.zoomToScale(imagePreviewSheet.pinchStartScale * event.pinchRatio,
                                                       imagePreviewSheet.pinchPivotX,
                                                       imagePreviewSheet.pinchPivotY,
                                                       imagePreviewSheet.pinchStartScale,
                                                       imagePreviewSheet.pinchStartOffsetX,
                                                       imagePreviewSheet.pinchStartOffsetY,
                                                       true);
                    }

                    onPinchEnded: {
                        imagePreviewSheet.isPinching = false;
                        imagePreviewSheet.settleImageTransform();
                    }
                },
                DoubleTapHandler {
                    onDoubleTapped: {
                        if (imagePreviewSheet.imageScale > 1.0) {
                            imagePreviewSheet.resetImageTransform();
                        } else {
                            imagePreviewSheet.zoomToScale(2.0,
                                                           imagePreviewSheet.eventX(event),
                                                           imagePreviewSheet.eventY(event));
                        }
                    }
                }
            ]
        }
    }

    function openUrl(url, imageWidth, imageHeight) {
        resetImageTransform();
        imagePreview.open(url, imageWidth, imageHeight);
        updateFittedImageSize();
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
        dragMoved = false;
        isPinching = false;
        pinchStartOffsetX = 0.0;
        pinchStartOffsetY = 0.0;
        pinchPivotX = 0.0;
        pinchPivotY = 0.0;
        updateFittedImageSize();
    }

    function settleImageTransform() {
        if (imageScale < 1.0) {
            resetImageTransform();
            return;
        }
        if (imageScale <= 1.0) {
            imageOffsetX = 0.0;
            imageOffsetY = 0.0;
        } else {
            clampImageOffset();
        }
    }

    function canPanImage() {
        return imageScale > 1.0 && (maxOffsetX() > 0.0 || maxOffsetY() > 0.0);
    }

    function zoomToScale(nextScale, pivotX, pivotY, baseScale, baseOffsetX, baseOffsetY, rubberBand) {
        updateFittedImageSize();

        if (baseScale === undefined || baseScale <= 0.0) {
            baseScale = imageScale;
        }
        if (baseOffsetX === undefined) {
            baseOffsetX = imageOffsetX;
        }
        if (baseOffsetY === undefined) {
            baseOffsetY = imageOffsetY;
        }
        if (pivotX === undefined || pivotX <= 0.0) {
            pivotX = previewWidth() / 2.0;
        }
        if (pivotY === undefined || pivotY <= 0.0) {
            pivotY = previewHeight() / 2.0;
        }

        if (rubberBand) {
            nextScale = rubberBandScale(nextScale);
        } else {
            nextScale = clamp(nextScale, 1.0, maxImageScale);
        }

        var scaleRatio = nextScale / baseScale;
        var centerX = previewWidth() / 2.0;
        var centerY = previewHeight() / 2.0;

        imageScale = nextScale;
        imageOffsetX = pivotX - centerX - scaleRatio * (pivotX - centerX - baseOffsetX);
        imageOffsetY = pivotY - centerY - scaleRatio * (pivotY - centerY - baseOffsetY);

        if (!rubberBand) {
            clampImageOffset();
        }
    }

    function rubberBandScale(value) {
        if (value < minImageScale) {
            return minImageScale;
        }
        if (value > maxImageScale) {
            return maxImageScale + (value - maxImageScale) * 0.25;
        }
        return value;
    }

    function rubberBandOffset(value, limit) {
        if (limit <= 0.0) {
            return value * 0.28;
        }
        if (value > limit) {
            return limit + (value - limit) * 0.28;
        }
        if (value < -limit) {
            return -limit + (value + limit) * 0.28;
        }
        return value;
    }

    function clampImageOffset() {
        updateFittedImageSize();

        if (imageScale <= 1.0) {
            imageOffsetX = 0.0;
            imageOffsetY = 0.0;
            return;
        }

        var maxOffsetX = imagePreviewSheet.maxOffsetX();
        var maxOffsetY = imagePreviewSheet.maxOffsetY();

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

    function maxOffsetX() {
        updateFittedImageSize();

        var offset = (fittedImageWidth * imageScale - previewWidth()) / 2.0;
        return offset > 0.0 ? offset : 0.0;
    }

    function maxOffsetY() {
        updateFittedImageSize();

        var offset = (fittedImageHeight * imageScale - previewHeight()) / 2.0;
        return offset > 0.0 ? offset : 0.0;
    }

    function clamp(value, minValue, maxValue) {
        if (value < minValue) {
            return minValue;
        }
        if (value > maxValue) {
            return maxValue;
        }
        return value;
    }

    function eventX(event) {
        if (event.x !== undefined) {
            return event.x;
        }
        if (event.localX !== undefined) {
            return event.localX;
        }
        if (event.windowX !== undefined) {
            return event.windowX;
        }
        return previewWidth() / 2.0;
    }

    function eventY(event) {
        if (event.y !== undefined) {
            return event.y;
        }
        if (event.localY !== undefined) {
            return event.localY;
        }
        if (event.windowY !== undefined) {
            return event.windowY;
        }
        return previewHeight() / 2.0;
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