import bb.cascades 1.4

Container {
    id: root

    property string author: ""
    property string initials: ""
    property string avatarColor: "#5865F2"
    property string time: ""
    property string message: ""
    property string replyAuthor: ""
    property string replyMessage: ""
    property string image: ""
    property int imageWidth: 0
    property int imageHeight: 0

    signal deleteRequested()
    signal replyRequested(string author, string message)

    horizontalAlignment: HorizontalAlignment.Fill
    leftPadding: ui.du(2.0)
    rightPadding: ui.du(2.0)
    topPadding: ui.du(1.5)
    bottomPadding: ui.du(1.0)

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
                        root.replyRequested(root.author, root.message)
                    }
                },

                ActionItem {
                    title: qsTr("Delete")
                    imageSource: "asset:///images/icons/sign-out.png"

                    onTriggered: {
                        root.deleteRequested()
                    }
                }
            ]
        }
    ]

    Container {
        preferredWidth: ui.du(8.0)
        verticalAlignment: VerticalAlignment.Top

        Container {
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
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        leftMargin: ui.du(1.5)

        Container {
            horizontalAlignment: HorizontalAlignment.Fill

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Label {
                text: root.author
                textStyle.fontSize: FontSize.Small
                textStyle.fontWeight: FontWeight.Normal
                textStyle.color: Color.create("#F2F3F5")
                verticalAlignment: VerticalAlignment.Center
            }

            Label {
                text: root.time
                leftMargin: ui.du(0.2)
                opacity: 0.5
                textStyle.fontSize: FontSize.XXSmall
                textStyle.color: Color.create("#C9CDD3")
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
            topMargin: ui.du(-0.6)
            multiline: true
            textStyle.fontSize: FontSize.XSmall
            textStyle.color: Color.create("#DCDDDE")
        }

        Container {
            visible: root.image !== ""
            preferredWidth: ui.du(root.imageWidth)
            preferredHeight: ui.du(root.imageHeight)
            topMargin: ui.du(1.0)

            layout: DockLayout {}

            ImageView {
                imageSource: root.image
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scalingMethod: ScalingMethod.AspectFit
            }
        }
    }
}