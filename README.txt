ABOUT MENU ITEM CONTAINER
-------------------------

By default, the Drupal menu system only allows you to add menu items that are
links. With this module, you can add menu items that are simply containers, and
not links.

This module works with breadcrumbs, core menu blocks, Menu block module, Menu
tree module, a theme's primary and secondary links variables, etc. Basically, if
it doesn't work in a particular place in Drupal, the other module or theme is
broken! (j/k. Please report any incompatibilities in the issue queue.)


ADDING MENU ITEM CONTAINERS
---------------------------

Just visit the Menus admisitration page at admin/build/menu and select the menu
you wish to add a container to. Above the list of links, next to the normal "Add
item" tab, you will see a new "Add container" tab.


STYLING CONTAINERS
------------------

By default, all container titles are wrapped in a <span> with the
"menu-item-container" class.

This module provides a theme function that can be overridden by a theme to
alter the HTML that is output. See the theme_menu_item_container() function.
This function can be overridden by the any of the provided theme hook
suggestions:

- theme_menu_item_container() can be overridden by creating one of:
  - [theme]_menu_item_container__[container id number]()
  - [theme]_menu_item_container__[menu name]()
  - [theme]_menu_item_container()

For example, if you created a garland_menu_item_container__primary_links()
function, it would override theme_menu_item_container() any time it was used in
the primary links menu, but not when used in any other menu. Similarly, a garland_menu_item_container__1() function would override
theme_menu_item_container(), but only for the first menu item container in your
system (the menu item container with an ID of 1).
