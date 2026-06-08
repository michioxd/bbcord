import bb.cascades 1.4

Container {
    id: root

    property string messageId: ""
    property string authorId: ""
    property string author: ""
    property string initials: ""
    property string avatarSource: ""
    property string avatarColor: "#5865F2"
    property string time: ""
    property real timestampMs: 0
    property string message: ""
    property string replyAuthor: ""
    property string replyMessage: ""
    property string image: ""
    property int imageWidth: 0
    property int imageHeight: 0
    property string attachmentUrl: ""
    property string attachmentName: ""
    property bool attachmentIsImage: false
    property bool isGroupStart: true
    property bool isGroupEnd: true
    property bool showAvatar: true
    property bool showUsername: true
    property bool showTimestamp: true
    property bool pending: false
    property bool failed: false
    property bool edited: false
    property bool imageLoadFailed: false
    property bool previewVisible: false
    property real maxImageWidthDu: 42.0
    property real maxImageHeightDu: 36.0
    property real minImageWidthDu: 14.0

    signal editRequested(string messageId, string message)
    signal deleteRequested(string messageId)
    signal replyRequested(string messageId, string author, string message)
    signal attachmentOpenRequested(string url)
    signal attachmentImageLoadRequested(string url)

    horizontalAlignment: HorizontalAlignment.Fill
    leftPadding: ui.du(2.0)
    rightPadding: ui.du(2.0)
    topPadding: root.isGroupStart ? ui.du(1.5) : ui.du(0.1)
    bottomPadding: root.isGroupEnd ? ui.du(1.0) : ui.du(0.2)

    layout: StackLayout {
        orientation: LayoutOrientation.LeftToRight
    }

    contextActions: [
        ActionSet {
            title: root.author
            subtitle: root.message

            actions: [
                ActionItem {
                    title: qsTr("Reply")

                    onTriggered: {
                        root.replyRequested(root.messageId, root.author, root.message);
                    }
                },
                ActionItem {
                    title: qsTr("Open attachment")
                    enabled: root.attachmentUrl !== ""

                    onTriggered: {
                        root.attachmentOpenRequested(root.attachmentUrl);
                    }
                },
                ActionItem {
                    title: qsTr("Edit")
                    enabled: !root.pending && !root.failed

                    onTriggered: {
                        root.editRequested(root.messageId, root.message);
                    }
                },
                ActionItem {
                    title: qsTr("Delete")
                    imageSource: "asset:///images/icons/sign-out.png"

                    onTriggered: {
                        root.deleteRequested(root.messageId);
                    }
                }
            ]
        }
    ]

    Container {
        preferredWidth: ui.du(8.0)
        verticalAlignment: VerticalAlignment.Top

        Container {
            visible: root.showAvatar && root.avatarSource === ""
            preferredWidth: ui.du(7.0)
            preferredHeight: ui.du(7.0)
            maxWidth: ui.du(7.0)
            minWidth: ui.du(7.0)
            maxHeight: ui.du(7.0)
            minHeight: ui.du(7.0)
            background: Color.create(root.avatarColor)

            layout: DockLayout {}

            Label {
                text: root.initials
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                textStyle.fontSize: FontSize.Small
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.White
            }
        }

        ImageView {
            visible: root.showAvatar && root.avatarSource !== ""
            imageSource: root.avatarSource
            preferredWidth: ui.du(7.0)
            preferredHeight: ui.du(7.0)
            maxWidth: ui.du(7.0)
            minWidth: ui.du(7.0)
            maxHeight: ui.du(7.0)
            minHeight: ui.du(7.0)
            scalingMethod: ScalingMethod.AspectFill
        }
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftMargin: ui.du(1.5)

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            visible: root.showUsername || root.showTimestamp

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Label {
                text: root.author
                visible: root.showUsername
                textStyle.fontSize: FontSize.Small
                textStyle.fontWeight: FontWeight.Normal
                textStyle.color: Color.create("#F2F3F5")
                verticalAlignment: VerticalAlignment.Center
            }

            Label {
                text: root.time
                visible: root.showTimestamp
                leftMargin: ui.du(0.2)
                opacity: 0.5
                textStyle.fontSize: FontSize.XXSmall
                textStyle.color: Color.create("#C9CDD3")
                verticalAlignment: VerticalAlignment.Center
            }

            Label {
                text: root.failed ? qsTr("failed") : (root.pending ? qsTr("sending") : (root.edited ? qsTr("edited") : ""))
                visible: root.failed || root.pending || root.edited
                leftMargin: ui.du(0.5)
                opacity: 0.55
                textStyle.fontSize: FontSize.XXSmall
                textStyle.color: root.failed ? Color.create("#ED4245") : Color.create("#C9CDD3")
                verticalAlignment: VerticalAlignment.Center
            }
        }

        Container {
            visible: root.replyAuthor !== ""
            horizontalAlignment: HorizontalAlignment.Fill
            topMargin: ui.du(0.2)
            bottomMargin: ui.du(0.8)

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Container {
                preferredWidth: ui.du(0.5)
                minWidth: ui.du(0.5)
                maxWidth: ui.du(0.5)
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create("#5865F2")
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                leftMargin: ui.du(1.0)
                leftPadding: ui.du(1.0)
                rightPadding: ui.du(1.0)
                topPadding: ui.du(0.6)
                bottomPadding: ui.du(0.6)
                background: Color.create('#151617')

                Label {
                    text: root.replyAuthor
                    textStyle.fontSize: FontSize.XXSmall
                    textStyle.fontWeight: FontWeight.Normal
                    textStyle.color: Color.create("#5865F2")
                }

                Label {
                    text: root.replyMessage
                    topMargin: ui.du(-0.3)
                    multiline: true
                    opacity: 0.85
                    textStyle.fontSize: FontSize.XXSmall
                    textStyle.color: Color.create("#B5BAC1")
                }
            }
        }

        Label {
            text: root.message
            topMargin: root.isGroupStart ? ui.du(-0.6) : ui.du(-0.2)
            multiline: true
            textStyle.fontSize: FontSize.XSmall
            textStyle.color: Color.create("#DCDDDE")
        }

        Container {
            visible: root.image !== ""
            preferredWidth: ui.du(root.displayImageWidth())
            preferredHeight: ui.du(root.displayImageHeight())
            maxWidth: ui.du(root.maxImageWidthDu)
            maxHeight: ui.du(root.maxImageHeightDu)
            topMargin: ui.du(1.0)

            layout: DockLayout {}

            ImageView {
                imageSource: root.image
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scalingMethod: ScalingMethod.AspectFit
            }
        }

        Button {
            visible: root.attachmentUrl !== "" && (!root.attachmentIsImage || root.imageLoadFailed)
            text: (root.attachmentIsImage ? qsTr("Load image: ") : qsTr("Open attachment: ")) + root.attachmentName
            topMargin: ui.du(1.0)
            horizontalAlignment: HorizontalAlignment.Left

            onClicked: {
                if (root.attachmentIsImage && root.image === "") {
                    root.attachmentImageLoadRequested(root.attachmentUrl);
                } else {
                    root.attachmentOpenRequested(root.attachmentUrl);
                }
            }
        }
    }

    function imageAspectRatio() {
        if (root.imageWidth > 0 && root.imageHeight > 0) {
            return root.imageWidth / root.imageHeight;
        }

        return 16.0 / 9.0;
    }

    function displayImageWidth() {
        var ratio = root.imageAspectRatio();
        var width = root.maxImageWidthDu;
        var height = width / ratio;

        if (height > root.maxImageHeightDu) {
            height = root.maxImageHeightDu;
            width = height * ratio;
        }

        if (width < root.minImageWidthDu) {
            width = root.minImageWidthDu;
        }

        return width;
    }

    function displayImageHeight() {
        var ratio = root.imageAspectRatio();
        var height = root.displayImageWidth() / ratio;

        if (height > root.maxImageHeightDu) {
            height = root.maxImageHeightDu;
        }

        return height;
    }
}
