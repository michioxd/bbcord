import bb.cascades 1.4

CustomListItem {
	property alias iconSource: icon.imageSource
	property alias title: titleLabel.text
	property alias description: descriptionLabel.text
	default property alias accessory: accessoryContainer.controls
	signal triggered()

	gestureHandlers: [
		TapHandler {
			onTapped: triggered()
		}
	]

	Container {
		horizontalAlignment: HorizontalAlignment.Fill
		leftPadding: ui.du(2.0)
		rightPadding: ui.du(2.0)

		layout: StackLayout {
			orientation: LayoutOrientation.LeftToRight
		}

		ImageView {
			id: icon
			scalingMethod: ScalingMethod.AspectFit
			preferredWidth: ui.du(10.0)
			preferredHeight: ui.du(10.0)
			verticalAlignment: VerticalAlignment.Center
            enabled: true
            visible: iconSource != ""
        }

		Container {
			leftMargin: ui.du(2.0)
			verticalAlignment: VerticalAlignment.Center
			horizontalAlignment: HorizontalAlignment.Fill

			layoutProperties: StackLayoutProperties {
				spaceQuota: 1
			}

			Label {
				id: titleLabel
				textStyle.fontSize: FontSize.Large
				horizontalAlignment: HorizontalAlignment.Fill
				bottomMargin: ui.du(0)
			}

			Label {
				id: descriptionLabel
				opacity: 0.55
				topMargin: ui.du(0)
				bottomMargin: ui.du(0)
				textStyle.fontSize: FontSize.Small
				multiline: true
			}
		}

		Container {
			id: accessoryContainer
			verticalAlignment: VerticalAlignment.Center

			layoutProperties: StackLayoutProperties {
				spaceQuota: -1
			}
		}
	}
}
