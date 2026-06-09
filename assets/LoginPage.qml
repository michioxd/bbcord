import bb.cascades 1.4
import bb.system 1.2

Page {
    signal loginSucceeded()

    function showLoginFailed(message) {
        loginFailToast.body = message.length > 0 ? message : qsTr("Login failed")
        loginFailToast.show()
    }

    Container {
        layout: DockLayout {}
        
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center

            layout: StackLayout {
            }

            leftPadding: ui.du(8.0)
            rightPadding: ui.du(8.0)

            ImageView {
                imageSource: "asset:///icon.png"
                horizontalAlignment: HorizontalAlignment.Center
                scalingMethod: ScalingMethod.AspectFit
                preferredWidth: ui.du(16.0)
                preferredHeight: ui.du(16.0)
            }

            Label {
                text: qsTr("Welcome to BBCord")
                horizontalAlignment: HorizontalAlignment.Center
            }

            TextField {
                id: tokenField
                hintText: qsTr("Enter your Discord token...")
                horizontalAlignment: HorizontalAlignment.Fill
                textFormat: TextFormat.Plain
                inputMode: TextFieldInputMode.Text
                text: ""
                visible: !appStore.busy
            }

            Button {
                id: btnLogin

                text: qsTr("Login")
                horizontalAlignment: HorizontalAlignment.Fill
                enabled: !appStore.busy
                visible: !appStore.busy

                onClicked: {
                    discordClient.login(tokenField.text)
                }
            }

            Button {
                id: btnContinue

                text: qsTr("Continue")
                horizontalAlignment: HorizontalAlignment.Fill
                visible: false

                onClicked: {
                    console.log("[login] continue clicked")
                    loginSucceeded()
                }
            }

            ActivityIndicator {
                running: appStore.busy
                visible: appStore.busy
                horizontalAlignment: HorizontalAlignment.Center
                topMargin: ui.du(4.0)
                preferredWidth: ui.du(8.0)
            }

            Label {
                text: appStore.statusText
                horizontalAlignment: HorizontalAlignment.Center
                multiline: true
                visible: text.length > 0
            }

            Button {
                id: btnInst
                text: qsTr("How to get token")
                horizontalAlignment: HorizontalAlignment.Fill
                visible: !appStore.busy
            }

            Label {
                text: "powered by michioxd"
                horizontalAlignment: HorizontalAlignment.Center
                textStyle.fontSizeValue: 0.0
                textFit.minFontSizeValue: 3.0
                textFit.maxFontSizeValue: 4.0
                textFit.mode: LabelTextFitMode.Default
                opacity: 0.5
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: loginFailToast
        }
    ]

    onCreationCompleted: {
        discordClient.loginFailed.connect(showLoginFailed)
    }
}