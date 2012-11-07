<?php

/**
 * Ajax callback for Shared Canvas
 * Supplies the basic data, including maifest uri
 * @global url $base_url
 * @param string $pid
 */
function islandora_canvas_setup($pid) {
  global $base_url;
  global $user;

  module_load_include('inc', 'islandora_image_annotation', 'includes/islandora_rdf_builder');
  module_load_include('module', 'taxonomy', 'module');
  $item = islandora_object_load($pid);
  $rels = $item->relationships->get('info:fedora/fedora-system:def/model#', 'hasModel');
  $cmodel = $rels[0]['object']['value'];
  $mappings = variable_get('islandora_annotation_mappings', array());
  $vid = $mappings[$cmodel]['TAX'];
  $terms = taxonomy_get_tree($vid,0,1);
  $categories = array();
  foreach($terms as $term){
    $categories[] = $term->name;
  }

  $title = $item->label;
  $pages = array();
  $pages[1] = $pid;
  $position = array_search($pid, $pages);
  $results = array();
  $results['manifest_url'] = "$base_url/emic/serve/$pid/Manifest/manifest.xml";
  $results['page_title'] = $item->label;
  $results['object_base'] = "$base_url/islandora/object/$pid";
  $results['islandora_post_url'] = "$base_url/emic/shared/add_annotation/$pid";
  $results['islandora_get_annotation'] = "$base_url/emic/get_annotation/";
  $results['islandora_delete_annotation'] = "$base_url/emic/delete_annotation/$pid/";
  $results['get_annotation_list_url'] = "$base_url/emic/get_urns/$pid";
  $results['islandora_update_annotation'] = "$base_url/emic/update_annotation/";
  $results['position'] = $position;
  $results['pages'] = $pages;
  $results['no_edit'] = !user_access(ISLANDORA_IMAGE_ANNOTATION_CREATE);
  $results['categories'] = $categories;
  $json = json_encode($results);
  echo $json;
}

/**
 * Creates annotation objects from html supplied by Shared Canvas
 * @global url $base_url
 * @param string $pid
 */
function islandora_add_annotation($pid) {
  global $user;
  global $base_url;
  //module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'emic_canvas', 'islandora_rdf_builder');
  module_load_include('inc', 'emic_canvas', 'build_sharedcanvas_streams');
  module_load_include('inc', 'islandora', 'includes/islandora.ingest');
  $annotation_xml = islandora_shared_transform_annotationList(urldecode($_POST['data']), 'html');
  $type = $_POST['type'];
  if (!$type) {
    $type = 'unclassified';
  }
  $xml = simplexml_load_string($annotation_xml);
  $labels = $xml->xpath('//dc:title');
  $annotations = $xml->xpath('//rdf:type[@rdf:resource="http://www.openannotation.org/ns/Annotation"]');
  $parents = $annotations[0]->xpath("parent::*");
  $parent = $parents[0];
  foreach ($parent->attributes('rdf', TRUE) as $key => $value) {
    $urn = (string) $value;
  }
  $namespace = substr($pid, 0, strpos($pid, ":"));
  $item = islandora_ingest_get_object('islandora:OACCModel', $pid, 'isMemberOf', $namespace);
  $item->label = (string) $labels[0];
  //relationships->add(FEDORA_MODEL_URI, 'hasModel', 'islandora:imageCModel');
  $item->relationships->add(FEDORA_MODEL_URI, 'hasModel', 'islandora:OACCModel');
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'targetedBy', $user->name, TRUE);
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'targets', $pid);
  $item->relationships->add('http://islandora.ca/ontology/relsext#', 'isAnnotationType', $type, TRUE);
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasURN', $urn, TRUE);
  //$item->add_datastream_from_string($annotation_xml, 'OAC', 'OAC', 'text/xml', 'X');
  $new_ds = $item->constructDatastream('OAC', 'X');
  $new_ds->label = 'OAC';
  $new_ds->mimetype = 'text/xml';
  $new_ds->content = $annotation_xml;
  $item->ingestDatastream($new_ds);
  islandora_ingest_add_object($item);
  echo $item->id;
}

/**
 * transforms input from SharedCanvas to AnnotationList
 * @param string $xml_in
 * @return string
 */
function islandora_shared_transform_annotationList($xml_in, $input_type) {

  if ($xml_in == NULL || strlen($xml_in) < 5) {
    return " ";
  }

  $path = drupal_get_path('module', 'islandora_image_annotation') . '/xslt/';
  $xslts = array(
    'html' => $path . 'html_to_AnnotationList.xsl',
    'rdf' => $path . 'AnnotationList_to_html.xsl'
  );
  try {
    $proc = new XsltProcessor();
  } catch (Exception $e) {
    drupal_set_message(t('@e', array('@e' => check_plain($e->getMessage()))), 'error');
    return " ";
  }

  $xsl = new DomDocument();
  $xsl->load($xslts[$input_type]);
  $input = new DomDocument();
  $input->loadXML(trim($xml_in));
  $xsl = $proc->importStylesheet($xsl);
  $newdom = $proc->transformToDoc($input);
  $return_xml = $newdom->saveXML();

  return $return_xml;
}

/**
 * Gets individual annotation pids from resource index
 * called with emic/get_urns
 * @global url $base_url
 * @param array $pids
 */
function islandora_shared_get_annotation_pids($pid) {
  $query = "select \$id from <#ri> where \$id <http://islandora.ca/ontology/relsext#targets> <info:fedora/$pid";

  $item = islandora_object_load($pid);

  $objects = $item->repository->ri->itqlQuery($query);

  foreach ($objects as $object) {
    $pids[] = $object['id']['value'];
  }
  $json = json_encode($pids);
  echo $json;
}

/**
 * Retrieves details of individual annotation from datastream
 * @global url $base_url
 * @param string $pid
 * 
 * @return IslandoraRdfBuilder
 */
function islandora_get_annotation_data($pid) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  global $base_url;
  $item = islandora_object_load($pid);
  if (!isset($item)) {
    return;
  }
  $annotation_xml = $item['OAC']->content;
  return $annotation_xml;
}

/**
 * Retrieves transforms annotation data for the editor
 * Called with /emic/get_annotation/
 * Doctype declarations are removed
 * @param string $pid
 * @param urn $urn
 */
function islandora_shared_get_annotation($pid) {

  $xml = islandora_get_annotation_data($pid);
  $results = islandora_shared_transform_annotationList($xml, 'rdf');
  $results = preg_replace('/<\?xml.*?\?>/', '', $results);
  $results = preg_replace('/<!DOCTYPE.*?>/', '', $results);

  echo $results;
}

/**
 *

 * @param string $pid
 * @param urn $urn
 */
function islandora_delete_annotation($pid, $urn) {
  // module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $pid_to_delete = islandora_canvas_get_pid_from_urn($urn);
  $item = islandora_object_load($pid_to_delete);
  $item->repository->purgeObject($item->id);
}

/**
 * Persists new annotations
 * @global url $base_url
 * @param <type> $pid
 * @param <type> $rdf
 */
function islandora_save_updated_annotations($pid, $rdf) {
  global $base_url;
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $item = islandora_object_load($pid);
  $updated_annotationList = $rdf->printRDF();
   $item['AnnotationList']->content = $updated_annotationList;
  
}

function build_annotationList($pid) {
  global $base_url;
  $doc_url = "$base_url/islandora/object/$pid";
  $doc_path_url = "$base_url/emic/serve/$pid";
  // module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'emic_canvas', 'build_sharedcanvas_streams');
  module_load_include('inc', 'emic_canvas', 'islandora_rdf_builder');
  $item = islandora_object_load($pid);

  $annotationList = get_annotationList($doc_url, $doc_path_url);
  $domDoc = new DOMDocument();
  $domDoc->formatOutput = true;
  $domDoc->loadXML($annotationList);
  $rdf = new IslandoraRdfBuilder($domDoc);

  if (isset($item['AnnotationList'])) {
    $list = $item['AnnotationList']->content;
    if (strlen($list) < 50) {
      return;
    }
    $annoDoc = new DOMDocument();
    $annoDoc->loadXML($list);
    $descriptions = $annoDoc->getElementsByTagName('Description');
    foreach ($descriptions as $description) {
      $constrains = $description->getElementsByTagName('constrains');
      foreach ($constrains as $candidate) {
        $candidate->setAttribute("rdf:resource", "$base_url/fedora/repository/$pid/Canvas");
      }
      $newNode = $domDoc->importNode($description, TRUE);
      $rdf->root->appendChild($newNode);
    }
  }

  return $rdf;
}

/**
 * Gets the pid of an annotation object identified by a know urn
 * @param URN $urn
 * @return PID
 */
function islandora_canvas_get_pid_from_urn($urn) {
  global $user;
  module_load_include('inc', 'islandora', 'includes/tuque');
  $connection = new IslandoraTuque($user);
  $query = "select \$id  from <#ri> where \$id <http://islandora.ca/ontology/relsext#hasURN> \$urn
and \$urn <mulgara:is> 'urn:uuid:$urn'";

  $objects = $connection->repository->ri->itqlQuery($query);
  $pid = $objects[0]['id']['value'];

  return $pid;
}

/**
 * Updates existing annotation
 * Called by Shared Canvas with ajax
 */
function islandora_canvas_update_annotation() {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $changed = FALSE;
  //retrieve variables from POST
  $urn = urldecode($_POST['urn']);
  $title = urldecode($_POST['title']);
  $content = urldecode($_POST['content']);
  $type = urldecode($_POST['annoType']);
  $pid = islandora_canvas_get_pid_from_urn($urn);

  $fedora_object = islandora_object_load($pid);
  $annotation = $fedora_object['OAC']->content;
  $xml = simplexml_load_string($annotation);

  // identify Title element and replace content
  $title_node = $xml->xpath("//rdf:Description[@rdf:about = 'urn:uuid:$urn']/dc:title");
  if ($title_node[0][0] != $title) {
    $title_node[0][0] = $title;
    $changed = TRUE;
  }

  $type_node = $xml->xpath("//rdf:Description[@rdf:about = 'urn:uuid:$urn']/dc:type");
  if ($type_node[0][0] != $type) {
    $type_node[0][0] = $type;
    $changed = TRUE;
  }

  //indentify URN of body to retrieve chars node to hold new annotation content
  $body_urn_array = $xml->xpath("//oac:hasBody");
  $body_urn = (string) $body_urn_array[0]->attributes('rdf', 'about');
  $content_node = $xml->xpath("//rdf:Description[@rdf:about = '$body_urn']//cnt:chars");
  if ((string) $content_node[0][0] != $content) {
    $content_node[0][0] = $content;
    $changed = TRUE;
  }


  // persist updated annotation
  if ($changed) {
    $fedora_object['OAC']->content = $xml->asXML();
  }
}