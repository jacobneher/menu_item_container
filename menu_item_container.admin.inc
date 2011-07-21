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
  $items['admin/structure/menu/manage/%menu/container/add'] = array(
    'title' => 'Add container',
    'page callback' => 'menu_item_container_edit',
    'page arguments' => array('add', NULL, 3),
    'access arguments' => array('administer menu'),
    'type' => MENU_LOCAL_ACTION,
    'file' => 'menu_item_container.admin.inc',
  );
  $items['admin/structure/menu/item/%menu_link/container/edit'] = array(
    'title' => 'Edit menu container',
    'page callback' => 'menu_item_container_edit',
    'page arguments' => array('edit', 4, NULL),
    'access arguments' => array('administer menu'),
    'type' => MENU_CALLBACK,
    'file' => 'menu_item_container.admin.inc',
  );
  $items['admin/structure/menu/item/%menu_link/container/delete'] = array(
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
        $form[$key]['title']['#markup'] = theme($hooks, $form[$key]['#item']);
        
        // Containers are always expanded.
        $form[$key]['expanded']['#disabled'] = TRUE;
        // Set the proper edit operations link.
        $form[$key]['operations']['edit']['#href'] = 'admin/structure/menu/item/' . $form[$key]['#item']['mlid'] . '/container/edit';
        // Build our own delete link
        $form[$key]['operations']['delete'] = array(
          '#type' => 'link',
          '#title' => t('delete'),
          '#href' => 'admin/structure/menu/item/' . $form[$key]['#item']['mlid'] . '/container/delete',
        );
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
  // We only need to alter the forms for menu item containers.
  if (arg(5) != 'container' && $form['module']['#value'] != 'menu_item_container') {
    return;
  }
  
  // Get the paramters of menu_edit_item().
  $type = arg(6);
  $item = $form['original_item']['#value'];

  if ($type == 'add') {
    // Find the next possible container ID so each path is unique.
    $result = db_select('menu_links', 'ml')
                ->fields('ml', array('link_path'))
                ->condition('module', 'menu_item_container')
                ->execute();
    $new_id = 1;
    while ($row = $result->fetchAssoc()) {
      $id = str_replace('menu-item-container/', '', $row['link_path']);
      if ($id >= $new_id) {
        $new_id = $id + 1;
      }
    }
    // This is an add form, initialize the menu item.
    $item = $form['original_item']['#value'];
    $item['link_path'] = 'menu-item-container/' . $new_id;
    $item['module'] = 'menu_item_container';
    $item['expanded'] = 1;
    foreach (array('link_path', 'module') as $key) {
      $form[$key] = array('#type' => 'value', '#value' => $item[$key]);
    }
    $form['original_item'] = array('#type' => 'value', '#value' => $item);
  }
  // Don't edit standard menu items.
  elseif ($item['module'] != 'menu_item_container') {
    return;
  }
  // Add an additional class to the form.
  $form['#attributes']['class'] .= ' menu-item-container-form';

  // Update the link_path item.
  unset($form['_path']);
  $form['link_path'] = array(
    '#type' => 'hidden',
    '#default_value' => $item['link_path'],
  );

  // Add a delete button, accessible on "edit" forms.
  $form['actions']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
    '#access' => $item['mlid'],
    '#submit' => array('menu_item_container_delete_submit'),
  );

  $form['link_title']['#title'] = t('Menu container title');
  $form['link_title']['#description'] = t('The text corresponding to this container that should appear in the menu.');

  $form['description']['#description'] = t('The description displayed when hovering over a menu container.');

  $form['enabled']['#description'] = t('Menu containers that are not enabled will not be listed in any menu.');

  $form['expanded']['#type'] = 'hidden';
  $form['expanded']['#default_value'] = 1;

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
  $item = &$form_state['values'];

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
  $info = $item['build_info']['args'][0];
  return confirm_form($form, t('Are you sure you want to delete the menu container %item?', array('%item' => $info['link_title'])), 'admin/structure/menu/manage/' . $info['menu_name']);
}

/**
 * Process menu delete form submissions.
 */
function menu_item_container_delete_form_submit($form, &$form_state) {
  $item = $form['#item']['build_info']['args'][0];
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
  $form_state['redirect'] = 'admin/structure/menu/item/' . $form_state['values']['mlid'] . '/container/delete';
}
