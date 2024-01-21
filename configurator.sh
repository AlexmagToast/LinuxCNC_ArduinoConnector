#!/bin/bash

# Function to display a menu using whiptail
show_menu() {
    whiptail --title "Sample Menu" --menu "Choose an option:" 15 50 4 \
        1 "Option 1" \
        2 "Option 2" \
        3 "Option 3" \
        4 "Exit" 3>&1 1>&2 2>&3
}

# Main script
while true; do
    choice=$(show_menu)

    case $choice in
        1)
            whiptail --msgbox "You chose Option 1" 10 30
            ;;
        2)
            whiptail --msgbox "You chose Option 2" 10 30
            ;;
        3)
            whiptail --msgbox "You chose Option 3" 10 30
            ;;
        4)
            whiptail --yesno "Do you really want to exit?" 10 30
            if [ $? -eq 0 ]; then
                exit 0
            fi
            ;;
        *)
            # Handle unexpected choices
            whiptail --msgbox "Invalid option, please try again." 10 30
            ;;
    esac
done
