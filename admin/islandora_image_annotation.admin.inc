<?php

function islandora_image_annotation_admin() {
  module_load_include('inc', 'islandora_image_annotation', 'admin/islandora_annotation_admin_util');
  drupal_add_css(drupal_get_path('module', 'islandora_image_annotation') . '/css/islandora_annotation.css');
  $form = array();
  $coptions = islandora_annotation_get_content_models();
  $selected = variable_get('islandora_annotation_mappings', NULL);
  $enforced = variable_get('islandora_annotation_enforce_taxonomy', 0);
  $chosen = array_keys($selected);
  $pids = array_keys($coptions);
  $alink_options = array(
    'attributes' => array('target' => '_blank'),
    'html' => TRUE,
  );
  foreach ($selected as $key => $value) {
    $label = isset($coptions[$key]) ? $coptions[$key] : "Unlabeled";
    unset($coptions['$key']);
    $coptions = array($key => $label) + $coptions;
  }

  $form['content_model_wrapper'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configure Content Models'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['content_model_wrapper']['annotation_table'] = array(
    '#type' => 'item',
    '#title' => t('Select Content Models'),
    '#description' => t('Allow annotation of objects associated with these content models.'),
    '#tree' => TRUE, // this attribute is important to return the submitted values in a deeper nested arrays in
    '#theme' => 'islandora_annotation_table',
  );



  foreach ($coptions as $pid => $label) {
    $dsids = get_annotation_dsids_from_dscomp($pid);
    $taxes = islandora_annotation_get_taxonomies();
    if (empty($dsids)) {
      continue;
    }
    $options[$pid] = '';
    // label
    $form['content_model_wrapper']['annotation_table']['name'][$pid] = array(
      '#type' => 'item',
      '#markup' => l($label, "islandora/object/$pid", $alink_options),
    );
    // pid
    $form['content_model_wrapper']['annotation_table']['content_model'][$pid] = array(
      '#type' => 'item',
      '#markup' => $pid,
    );

    // dsid


    $form['content_model_wrapper']['annotation_table']['DSID'][$pid] = array(
      '#prefix' => '<div class="dsidSelect">',
      '#suffix' => '</div>',
      '#type' => 'select',
      '#options' => $dsids,
      '#default_value' => isset($selected[$pid]['DSID']) ? $selected[$pid]['DSID'] : '',
    );

    $form['content_model_wrapper']['annotation_table']['TAX'][$pid] = array(
      '#prefix' => '<div class="dsidSelect">',
      '#suffix' => '</div>',
      '#type' => 'select',
      '#options' => $taxes,
      '#default_value' => isset($selected[$pid]['TAX']) ? $selected[$pid]['TAX'] : '',
    );
    
    $form['content_model_wrapper']['annotation_table']['enabled'] = array(
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => $chosen,
    );
  }


  $form['annotation_categories'] = array(
    '#type' => 'radios',
    '#title' => t('Annotation Categories'),
    '#options' => array(0 => t('User defined'), 1 => t("Administrator defined")),
    '#description' => t('Allows administrators to select predefined taxonomies to predefine annotation categorization terminology'),
    '#default_value' => $enforced,
  );

  $form['actions'] = array(
    '#type' => 'actions'
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Configuration'),
    '#weight' => 0,
    '#submit' => array('islandora_image_annotation_admin_submit'),
  );


  return($form);
}

function islandora_image_annotation_admin_submit($form, &$form_state) {
  $enabled = array_filter($form_state['values']['annotation_table']['enabled']);

  $content_model_mappings = array();
  foreach ($enabled as $label => $pid) {
    $content_model_mappings[$pid]['DSID'] = $form_state['values']['annotation_table']['DSID'][$pid];
    $content_model_mappings[$pid]['TAX'] = $form_state['values']['annotation_table']['TAX'][$pid];
  }
  variable_set('islandora_annotation_mappings', $content_model_mappings);
  variable_set('islandora_annotation_enforce_taxonomy', $form_state['values']['annotation_categories']);
  
}

