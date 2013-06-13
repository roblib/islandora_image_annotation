<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * queries the accesslog table to find recently viewed image annotations 
 * accesslog is depentant on the statistics module being enabled
 * @global type $user
 */
function islandora_image_annotation_get_recently_viewed() {
  if(!module_exists('statistics')){
    drupal_set_message(t('No recently viewed annotations found.  The Statistics module 
      must be enabled for the recent views to function.'), 'info');
    return '';
  }
  $limit = variable_get('islandora_image_annotation_recent_views_limit', 15);
  global $user;
  global $base_url;
  $url_limiter = '%islandora/object/%/annotation'; // check the url for this term, to only show certain pages in the list
  if ($user->uid == 'Anonymous') {
    // anonymous user,  track using the hostname
    $result = db_query("SELECT DISTINCT title, path FROM {accesslog} WHERE uid = 'Anonymous' AND hostname = :hostname AND path LIKE :url_limiter ORDER BY timestamp DESC LIMIT $limit", array(':hostname' => $user->hostname, ':url_limiter' => $url_limiter));
  }
  else {
    $result = db_query("SELECT DISTINCT title, path FROM {accesslog} WHERE uid = :uid AND path LIKE :url_limiter ORDER BY timestamp DESC LIMIT $limit", array(':uid' => $user->uid, ':url_limiter' => $url_limiter));
  }
  foreach ($result as $row) {
    $thumbnail_url = $base_url . '/' . str_replace('annotation', 'datastream/TN/view', $row->path);
    $thumbnail = "<img src = '$thumbnail_url'/>";
    $links[] = array('TN' => $thumbnail, 'link' => l($row->title ? $row->title : t('No Title'), $row->path),
      'title' => $row->title ? $row->title : t('No Title'), 'url_only' => $base_url . '/' . $row->path);
  }
  if (empty($links)) {
    drupal_set_message(t('No recently viewed annotations found.'), 'info');
    return " ";
  }

  return theme('islandora_image_annotation_recent_views', array('items' => $links));
}

?>
