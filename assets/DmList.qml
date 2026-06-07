import bb.cascades 1.4

Container {
	id: dmList

	signal dmSelected(string channelName)

	horizontalAlignment: HorizontalAlignment.Fill
	verticalAlignment: VerticalAlignment.Fill

	layout: StackLayout {}

Container {
        preferredHeight: ui.du(10)
        leftPadding: ui.du(2)
        rightPadding: ui.du(2)
        background: Color.create("#111111")
        horizontalAlignment: HorizontalAlignment.Fill

        layout: StackLayout {
            orientation: LayoutOrientation.LeftToRight
        }

        Label {
            text: "Direct Messages"
            verticalAlignment: VerticalAlignment.Center
            textStyle.fontSize: FontSize.Medium
            textStyle.fontWeight: FontWeight.Normal
            horizontalAlignment: HorizontalAlignment.Fill
        }
    }

	ListView {
		dataModel: dmModel

		onTriggered: {
			var item = dmModel.data(indexPath)

			if (item.type == "dm") {
				dmList.dmSelected(item.name)
			}
		}

		listItemComponents: [
			ListItemComponent {
				type: "category"

				Container {
					preferredHeight: 2.0
					horizontalAlignment: HorizontalAlignment.Fill
					leftPadding: ui.du(2)
					rightPadding: ui.du(2)
					topPadding: ui.du(2.0)

					Label {
						text: ListItemData.name.toUpperCase()
						verticalAlignment: VerticalAlignment.Center
						opacity: 0.45
						textStyle.fontSize: FontSize.XSmall
					}
				}
			},

			ListItemComponent {
				type: "dm"

				Container {
					preferredHeight: ui.du(8.0)
					horizontalAlignment: HorizontalAlignment.Fill
					leftPadding: ui.du(2)
					rightPadding: ui.du(2)

					layout: StackLayout {
						orientation: LayoutOrientation.LeftToRight
					}

					Container {
						preferredWidth: ui.du(6.0)
						preferredHeight: ui.du(6.0)
						minWidth: ui.du(6.0)
						minHeight: ui.du(6.0)
						maxWidth: ui.du(6.0)
						maxHeight: ui.du(6.0)
						verticalAlignment: VerticalAlignment.Center
						background: Color.create(ListItemData.avatarColor)

						layout: DockLayout {}

						ImageView {
							imageSource: ListItemData.avatar
							visible: ListItemData.avatar !== ""
							horizontalAlignment: HorizontalAlignment.Fill
							verticalAlignment: VerticalAlignment.Fill
							scalingMethod: ScalingMethod.AspectFill
						}

						Label {
							text: ListItemData.initials
							visible: ListItemData.avatar === ""
							horizontalAlignment: HorizontalAlignment.Center
							verticalAlignment: VerticalAlignment.Center
							textStyle.fontSize: FontSize.Small
							textStyle.fontWeight: FontWeight.Bold
							textStyle.color: Color.White
						}
					}

					Label {
						text: ListItemData.name
						leftMargin: ui.du(1.5)
						verticalAlignment: VerticalAlignment.Center
						textStyle.fontSize: FontSize.Medium
					}
				}
			}
		]

		function itemType(data, indexPath) {
			return data.type
		}
	}

	attachedObjects: [
		ArrayDataModel {
			id: dmModel
		}
	]

	onCreationCompleted: {
		dmModel.append({"type": "category", "name": "Friends"})
		dmModel.append({"type": "dm", "name": "michioxd", "initials": "M", "avatarColor": "#5865F2", "avatar": ""})
		dmModel.append({"type": "dm", "name": "BerryBot", "initials": "B", "avatarColor": "#43B581", "avatar": ""})
		dmModel.append({"type": "dm", "name": "Demo User", "initials": "D", "avatarColor": "#4F545C", "avatar": "asset:///images/demo.png"})
	}
}
