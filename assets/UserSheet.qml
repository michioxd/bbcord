import bb.cascades 1.4

Sheet {
    id: userSheet

    Page {
        titleBar: TitleBar {
            title: qsTr("Me")
            dismissAction: ActionItem {
                imageSource: "asset:///images/icons/accent/caret-left.png"
                onTriggered: userSheet.close()
            }
        }

        Container {
            leftPadding: ui.du(4)
            rightPadding: ui.du(4)
            topPadding: ui.du(4)

            layout: StackLayout {}

            ImageView {
                imageSource: appStore.currentUserAvatarSource
                horizontalAlignment: HorizontalAlignment.Center
                scalingMethod: ScalingMethod.AspectFit
                preferredWidth: ui.du(32.0)
                preferredHeight: ui.du(32.0)
            }

            Label {
                text: appStore.currentUserName.length > 0 ? appStore.currentUserName : qsTr("Not logged in")
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.fontSize: FontSize.Large
            }
            
            Label {
                text: appStore.currentUserTag
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.fontSize: FontSize.XSmall
                opacity: 0.5
                topMargin: ui.du(-0.5)
            }

            Divider {}

            Label {
                text: qsTr("User ID: ") + appStore.currentUserId
                visible: appStore.currentUserId.length > 0
                multiline: true
            }
        }
    }
}
