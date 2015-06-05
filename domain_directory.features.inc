<?php

/**
 * @file
 * Features support for Domain Alias.
 */

/**
 * Implements hook_features_export_options().
 */
function domain_directory_features_export_options() {
  return domain_features_get_options();
}

/**
 * Implements hook_features_export().
 */
function domain_directory_features_export($data, &$export, $module_name) {
  $export['dependencies']['domain_directory'] = 'domain_directory';
  $list = domain_features_selection($data);
  foreach ($list as $machine_name) {
    $export['features']['domain_directory'][$machine_name] = $machine_name;
  }
  domain_features_export_set_wipe_tables($export, $data, 'domain_directory');
  return array();
}

/**
 * Implements hook_features_export_render().
 */
function domain_directory_features_export_render($module_name, $data, $export = NULL) {
  domain_features_load($module_name, 'domain_directory_defaults', FALSE);
  $code = array();
  $code[] = '  $domain_directories = array();';
  // Set the wipe tables item.
  if ($wipe = domain_features_export_wipe_tables_code($data, $code, $export, 'domain_directories') && empty($export)) {
    // Check for changes against the target database.
    $data = domain_machine_names();
  }
  foreach ($data as $name) {
    if ($name != 'wipe-domain-tables') {
      $list = domain_directory_prepare_export($name);
      $code[] = "  \$domain_directories['{$name}'] = " . features_var_export($list, '  ') . ";";
    }
  }
  $code[] = "\n  return \$domain_directories;";
  $output = implode("\n", $code);
  return array('domain_directory_defaults' => $output);
}

/**
 * Prepares the export for a domain.
 *
 * @param $machine_name
 *  The source domain machine name.
 *
 * @return
 *  An array of defaults to export.
 */
function domain_directory_prepare_export($machine_name) {
  $domain = domain_machine_name_load($machine_name);
  $domain = domain_api($domain);

  return array('directory' => isset($domain['directory']) ? $domain['directory'] : NULL);
}

/**
 * Implements hook_features_revert().
 */
function domain_directory_features_revert($module) {
  return domain_directory_features_rebuild($module);
}

/**
 * Implements hook_features_rebuild().
 */
function domain_directory_features_rebuild($module) {
  if ($defaults = domain_features_load($module, 'domain_directory_defaults', TRUE)) {
    // Check for hard rebuild/revert.
    if ($wipe = domain_features_wipe_tables($defaults)) {
      db_delete('domain_directory')
        ->execute();
      unset($defaults['wipe-domain-tables']);
    }
    foreach ($defaults as $key => $list) {
      $domain_id = domain_load_domain_id($key);
      if (!$domain_id) {
        continue;
      }
      // Delete existing records.
      db_delete('domain_directory')
        ->condition('domain_id', $domain_id)
        ->execute();
      // Save the new alias records.
      if (!empty($list)) {
        $record = array(
          'domain_id' => $domain_id,
          'directory' => $list['directory'],
          'is_default' => 0,
        );
        drupal_write_record('domain_directory', $record);
      }
    }
  }
}
