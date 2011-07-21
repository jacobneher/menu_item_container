<?php
/**
 * @file
 * Provides infrequently-used functions for menu item containers.
 */

/**
 * Implements hook_menu().
 */
function _menu_item_container_menu() {
  $items['menu-item-container'] = array(
    'title' => 'Home',
    'description' => 'Container for menu link items.',
    'access arguments' => array('access content'),
    'page callback' => 'menu_item_container',
    'type' => MENU_CALLBACK,
  );
  $items['admin/structure/menu/manage/%menu/container-add'] = array(
    'title' => 'Add container',
    'page callback' => 'menu_item_container_edit',
    'page arguments' => array('add', NULL, 3),
    'access arguments' => array('administer menu'),
    'type' => MENU_LOCAL_ACTION,
    'file' => 'menu_item_container.admin.inc',
  );
  $items['admin/structure/menu/item/%menu_link/container-edit'] = array(
    'title' => 'Edit menu container',
    'page callback' => 'menu_item_container_edit',
    'page arguments' => array('edit', 4, NULL),
    'access arguments' => array('administer menu'),
    'type' => MENU_CALLBACK,
    'file' => 'menu_item_container.admin.inc',
  );
  $items['admin/structure/menu/item/%menu_link/container-delete'] = array(
    'title' => 'Delete menu container',
    'page callback' => 'menu_item_container_delete_page',
    'page arguments' => array(4),
    'access arguments' => array('administer menu'),
    'type' => MENU_CALLBACK,
    'file' => 'menu_item_container.admin.inc',
  );

  return $items;
}

/**
 * Sets container-specific elements on the menu overview form.
 */
function _menu_item_container_form_menu_overview_form_alter(&$form, &$form_state) {
  foreach (element_children($form) as $key) {
    if (isset($form[$key]['#item'])) {
      list($module, $id, ) = explode('/', $form[$key]['#item']['link_path'] . '//', 3);
      if ($module == 'menu-item-container') {
        // Replace the default link with a properly themed container.
        $hooks = array(
          'menu_item_container__' . $id,
          'menu_item_container__' . str_replace('-', '_', $form[$key]['#item']['menu_name']),
          'menu_item_container',
        );
        $form[$key]['title']['#value'] = theme($hooks, $form[$key]['#item']);
        // Containers are always expanded.
        $form[$key]['expanded']['#disabled'] = TRUE;
        // Set the proper operations links.
        $form[$key]['operations']['edit']['#value'] = l('edit', 'admin/structure/menu/item/' . $form[$key]['#item']['mlid'] . '/container-edit');
        $form[$key]['operations']['delete']['#value'] = l('delete', 'admin/structure/menu/item/' . $form[$key]['#item']['mlid'] . '/container-delete');
      }
    }
  }
}

/**
 * Menu callback; Build the menu container editing form.
 *
 * @see menu_item_container_form_menu_edit_item_alter()
 */
function menu_item_container_edit($type, $item, $menu) {
  module_load_include('inc', 'menu', 'menu.admin');
  return drupal_get_form('menu_edit_item', $type, $item, $menu, 'menu_item_container_edit');
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function _menu_item_container_form_menu_edit_item_alter(&$form, &$form_state) {
  // Get the paramters of menu_edit_item().
  list(,, $type, $item, $menu) = $form['#parameters'];

  // We only need to alter the forms for menu item containers.
  if (!isset($form['#parameters'][5]) || $form['#parameters'][5] != 'menu_item_container_edit' || isset($item['module']) && $item['module'] != 'menu_item_container') {
    return;
  }

  if ($type == 'add') {
    // Find the next possible container ID so each path is unique.
    $result = db_query('SELECT link_path FROM {menu_links} WHERE module = "menu_item_container"');
    $new_id = 1;
    while ($row = db_fetch_array($result)) {
      $id = str_replace('menu-item-container/', '', $row['link_path']);
      if ($id >= $new_id) {
        $new_id = $id + 1;
      }
    }
    // This is an add form, initialize the menu item.
    $item = $form['menu']['original_item']['#value'];
    $item['link_path'] = 'menu-item-container/' . $new_id;
    $item['module'] = 'menu_item_container';
    $item['expanded'] = 1;
    foreach (array('link_path', 'module') as $key) {
      $form['menu'][$key] = array('#type' => 'value', '#value' => $item[$key]);
    }
    $form['menu']['original_item'] = array('#type' => 'value', '#value' => $item);
  }
  // Don't edit standard menu items.
  elseif ($item['module'] != 'menu_item_container') {
    return;
  }

  // Add an additional class to the fieldset.
  $form['menu']['#attributes']['class'] .= ' menu-item-container-form';

  // Update the link_path item.
  unset($form['menu']['_path']);
  $form['menu']['link_path'] = array(
    '#type' => 'hidden',
    '#default_value' => $item['link_path'],
  );

  // Add a delete button, accessible on "edit" forms.
  $form['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
    '#access' => $item['mlid'],
    '#submit' => array('menu_item_container_delete_submit'),
    '#weight' => 10,
  );

  $form['menu']['link_title']['#title'] = t('Menu container title');
  $form['menu']['link_title']['#description'] = t('The text corresponding to this container that should appear in the menu.');

  $form['menu']['description']['#description'] = t('The description displayed when hovering over a menu container.');

  $form['menu']['enabled']['#description'] = t('Menu containers that are not enabled will not be listed in any menu.');

  $form['menu']['expanded']['#type'] = 'hidden';
  $form['menu']['expanded']['#default_value'] = 1;

  // Override validate and submit handlers.
  $key = array_search('menu_edit_item_validate', $form['#validate']);
  unset($form['#validate'][$key]);
  $key = array_search('menu_edit_item_submit', $form['#submit']);
  $form['#submit'][$key] = 'menu_item_container_edit_submit';

  return $form;
}

/**
 * Process menu and menu item add/edit form submissions.
 */
function menu_item_container_edit_submit($form, &$form_state) {
  $item = &$form_state['values']['menu'];

  // The value of "hidden" is the opposite of the value
  // supplied by the "enabled" checkbox.
  $item['hidden'] = (int) !$item['enabled'];
  unset($item['enabled']);

  $item['options']['attributes']['title'] = $item['description'];
  list($item['menu_name'], $item['plid']) = explode(':', $item['parent']);
  if (!menu_link_save($item)) {
    drupal_set_message(t('There was an error saving the menu container.'), 'error');
  }
  $form_state['redirect'] = 'admin/structure/menu/manage/' . $item['menu_name'];
}

/**
 * Menu callback; Check access and present a confirm form for deleting a menu link.
 */
function menu_item_container_delete_page($item) {
  // Links defined via hook_menu may not be deleted. Updated items are an
  // exception, as they can be broken.
  if ($item['module'] == 'system' && !$item['updated']) {
    drupal_access_denied();
    return;
  }
  return drupal_get_form('menu_item_container_delete_form', $item);
}

/**
 * Build a confirm form for deletion of a single menu link.
 */
function menu_item_container_delete_form(&$form_state, $item) {
  $form['#item'] = $item;
  return confirm_form($form, t('Are you sure you want to delete the menu container %item?', array('%item' => $item['link_title'])), 'admin/structure/menu/manage/' . $item['menu_name']);
}

/**
 * Process menu delete form submissions.
 */
function menu_item_container_delete_form_submit($form, &$form_state) {
  $item = $form['#item'];
  menu_link_delete($item['mlid']);
  $t_args = array('%title' => $item['link_title']);
  drupal_set_message(t('The menu container %title has been deleted.', $t_args));
  watchdog('menu', 'Deleted menu container %title.', $t_args, WATCHDOG_NOTICE);
  $form_state['redirect'] = 'admin/structure/menu/manage/' . $item['menu_name'];
}

/**
 * Submit function for the delete button on the menu container editing form.
 */
function menu_item_container_delete_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/structure/menu/item/' . $form_state['values']['menu']['mlid'] . '/container-delete';
}
