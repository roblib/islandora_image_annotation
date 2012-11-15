<?php

/**
 * @file
 * Class override for grid display
 */

/**
 * Extention of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsAnno extends IslandoraSolrResults {

  /**
   * Renders the primary or secondary display profile based on the raw Solr
   * results. This is the method most Islandora Solr display plugins would
   * override.
   *
   * @param type $results
   *   The raw Solr results from IslandoraSolrQueryProcessor::solrResult
   * @return
   *   Rendered Solr results
   *
   * @see IslandoraSolrResults::displayResults()
   */
  function printResults($results) {

    // set variables
    $results_array = array();
    $elements = array();

    // total
    $elements['solr_total'] = $results->response->numFound;

    // start
    $elements['solr_start'] = $results->response->start;

    // get prepared search results
    $docs = $this->prep_results($results);

    // optionally limit results to values given
    $limitResults = variable_get('islandora_solr_limit_result_fields', 0);
    // we will honor the limit results for display but we need some fields 
    // like PID and content model, the content model so we can determine what object to link to
    $key_fields = array();

    // loop over results array
    foreach ($docs as $doc) {
      $rows = array();
      $row = 0; // keeping $row in there for now until the snippet issue is cleared out
      $snippet = NULL; // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      $this->add_key_field($key_fields, 'rels.hasModel', $doc);
      $this->add_key_field($key_fields, 'PARENT_pid', $doc);      
      // loop over individual results to process fields
      foreach ($doc as $field => $value) {
        if ($field == 'PID') {
          $rows[$field]['label'] = 'Object ID'; //we need the pid regardless, the label may get changed below
        }
        // check if this field should be included
        if ($limitResults && empty($this->resultFieldArray[$field])) {
          continue;
        }

        // check for field label substitutions
        $translated_field_name = isset($this->allSubsArray[$field]) ? $this->allSubsArray[$field] : $field;

        // add label
        $rows[$field]['label'] = $translated_field_name;

        // add class: render solr field name to string that can be used as class name
        $rows[$field]['class'] = drupal_strtolower(preg_replace('/[^A-Za-z0-9]/', '-', $field));

        // split value if the result value is an array
        if (is_array($value)) {
          $value = implode(", ", $value);
        }
        // add value
        $rows[$field]['value'] = $value;

        $row++; // keeping $row in there for now until the snippet issue is cleared out
      }
      // @TODO: how is this suposed to work if the $snippet always is set to NULL ?
      if ($snippet) {
        $rows[$row][] = array(
          'data' => 'Full Text',
          'header' => TRUE,
        );
        $rows[$row][] = $snippet[0];
      }
      // append array to results array
      $results_array[] = $rows;
    }

    // return themed search results
    return theme('islandora_image_annotation_solr', array('results' => $results_array, 'elements' => $elements, 'key_fields' => $key_fields));
  }

  /**
   * 
   * @param array $key_fields
   * @param string $field
   * @param array $doc 
   */
  function add_key_field(&$key_fields, $field, $doc) {
    if (isset($doc[$field])) {
      foreach ($doc[$field] as $key => $value) {
        $key_fields[$field][$doc['PID']] = $value;
      }
    }
  }

}

