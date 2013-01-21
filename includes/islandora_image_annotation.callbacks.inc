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
  $stroke_width = $item->relationships->get(ISLANDORA_RELS_EXT_URI, 'strokeWidth');
  $stroke_width = $stroke_width[0]['object']['value'];
  if(empty($stroke_width)){
    $stroke_width = variable_get('annotation_stroke_width', 1);
  }
  $mappings = variable_get('islandora_annotation_mappings', array());
  $optimized = variable_get('islandora_annotation_optimized', TRUE);
  $dropdown = variable_get('islandora_annotation_enforce_taxonomy', TRUE);
  $vid = $mappings[$cmodel]['TAX'];
  $dsid = $mappings[$cmodel]['DSID'];
  if ($dropdown) {
    $terms = taxonomy_get_tree($vid, 0, 1);
    $categories = array();
    foreach ($terms as $term) {
      $categories[] = $term->name;
    }
  }
  else {
    $categories = islandora_annotations_get_type_terms();
  }

  $image_url = "$base_url/islandora/object/$pid/datastream/$dsid";
  $title = $item->label;
  $pages = array();
  $pages[1] = $pid;
  $position = array_search($pid, $pages);
  $results = array();
  $results['manifest_url'] = "$base_url/islandora/anno/serve/$pid/Manifest/manifest.xml";
  $results['page_title'] = $item->label;
  $results['object_base'] = "$base_url/islandora/object/$pid";
  $results['islandora_post_url'] = "$base_url/islandora/anno/add_annotation/$pid";
  $results['islandora_get_annotation'] = "$base_url/islandora/anno/get_annotation/";
  $results['islandora_delete_annotation'] = "$base_url/islandora/anno/delete_annotation/$pid/";
  $results['get_annotation_list_url'] = "$base_url/islandora/anno/get_urns/$pid";
  $results['islandora_update_annotation'] = "$base_url/islandora/anno/update_annotation/";
  $results['islandora_base_url'] = "$base_url/";
  $results['islandora_anno_use_title_vocab'] = variable_get('annotation_use_title_vocabulary', FALSE);
  $results['islandora_anno_stroke_width'] = $stroke_width;//default
  $results['islandora_anno_stroke_widths'] = variable_get('annotation_stroke_widths', '.3 .6. .9  1.2 1.5');
  $results['position'] = $position;
  $results['pages'] = $pages;
  $results['no_edit'] = !user_access(ISLANDORA_IMAGE_ANNOTATION_CREATE);
  $results['categories'] = $categories;
  $results['image_url'] = $image_url;
  $results['can_choose'] = !$optimized;
  $results['use_dropdown'] = $dropdown;
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
  module_load_include('inc', 'islandora', 'includes/utilities');
  $data = $_POST['data'];
  $stroke_width = $_POST['strokeWidth'];
  $annotation_xml = islandora_shared_transform_annotationList(urldecode($_POST['data']), 'html');
  $color = $_POST['color'];
  $type = $_POST['type'];
  if (!$type) {
    $type = 'unclassified';
  }
  $xml = simplexml_load_string($annotation_xml);
  $labels = $xml->xpath('//dc:title');
  $annotations = $xml->xpath('//rdf:type[@rdf:resource="http://www.w3.org/ns/openannotation/core/Annotation"]');
  $parents = $annotations[0]->xpath("parent::*");
  $parent = $parents[0];
  foreach ($parent->attributes('rdf', TRUE) as $key => $value) {
    $urn = (string) $value;
  }
  $namespace = substr($pid, 0, strpos($pid, ":"));
  //$item = islandora_ingest_get_object('islandora:OACCModel', $pid, 'isMemberOf', $namespace);
  $relationship = array();
  $relationship['relationship'] = 'isMemberOf';
  $relationship['pid'] = $pid;
  $item = islandora_prepare_new_object($namespace, $labels[0], $datastreams = array(), array('islandora:OACCModel'), array($relationship));
  $item->label = (string) $labels[0];

  //$item->relationships->add(FEDORA_MODEL_URI, 'hasModel', 'islandora:OACCModel');
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'targetedBy', $user->name, TRUE);
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'targets', $pid);
  if ($color) {
    $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasColor', $color, TRUE);
  }
  $item->relationships->add(FEDORA_RELS_EXT_URI, 'isAnnotationOf', $pid);
  $item->relationships->add('http://islandora.ca/ontology/relsext#', 'isAnnotationType', $type, TRUE);
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasURN', $urn, TRUE);
  $item->relationships->add(ISLANDORA_RELS_EXT_URI, 'strokeWidth', $stroke_width, TRUE);
  $new_ds = $item->constructDatastream('OAC', 'X');
  $new_ds->label = 'OAC';
  $new_ds->mimetype = 'text/xml';
  $new_ds->content = $annotation_xml;
  $item->ingestDatastream($new_ds);
  //islandora_ingest_add_object($item);
  $item->repository->ingestObject($item);
  foreach (islandora_build_hook_list(ISLANDORA_POST_INGEST_HOOK, $object->models) as $hook) {
    module_invoke_all($hook, $item);
  }
  echo $item->id;
}

function islandora_ingest_add_object(&$object) {
  drupal_alter('islandora_pre_ingest', $object);

  $object->repository->ingestObject($object);

  module_load_include('inc', 'islandora', 'includes/utilities');

  foreach (islandora_build_hook_list(ISLANDORA_POST_INGEST_HOOK, $object->models) as $hook) {
    module_invoke_all($hook, $object);
  }

  return $object;
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
 * called with islandora/anno/get_urns
 * @global url $base_url
 * @param array $pids
 */
function islandora_shared_get_annotation_pids($pid) {

  $query = <<<QUERY
PREFIX io: <http://islandora.ca/ontology/relsext#>
PREFIX fm: <info:fedora/fedora-system:def/model#>


SELECT ?id ?type ?color ?strokeWidth ?urn from <#ri>
WHERE {
        ?id io:targets <info:fedora/$pid> ;
            io:hasURN ?urn;
            io:isAnnotationType ?type;
            fm:state fm:Active ;

            OPTIONAL{
            ?id io:hasColor ?color
            }
             OPTIONAL{
             ?id io:strokeWidth ?strokeWidth;
            }
}
ORDER BY ASC(?type)
QUERY;

  $item = islandora_object_load($pid);
  $objects = $item->repository->ri->sparqlQuery($query);

  foreach ($objects as $object) {
    $itemdata = array(
      'id' => $object['id']['value'],
      'type' => $object['type']['value'],
      'color' => $object['color']['value'],
      'strokeWidth' => $object['strokeWidth']['value'],
      'urn' => $object['urn']['value'],
    );

    $pids['pids'][] = $itemdata;
    $pids['types'][] = $object['type']['value'];
  }
  $pids['types'] = array_values(array_unique($pids['types']));
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
 * Called with /islandora/anno/get_annotation/
 * Doctype declarations are removed
 * @param string $pid
 * @param urn $urn
 */
function islandora_shared_get_annotation($pid, $urn=NULL) {
  if(isset($urn)){
    $pid = islandora_canvas_get_pid_from_urn($urn);
  }
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
  $doc_path_url = "$base_url/islandora/anno/serve/$pid";
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
  //module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  $changed = FALSE;
  //retrieve variables from POST
  $urn = urldecode($_POST['urn']);
  $title = urldecode($_POST['title']);
  $content = urldecode($_POST['content']);
  $type = urldecode($_POST['annoType']);
  $new_color = urldecode($_POST['color']);
  $new_stroke_width = urldecode($_POST['strokeWidth']);
  $annotation_xml = islandora_shared_transform_annotationList(urldecode($_POST['rdfData']), 'html');
  $pid = islandora_canvas_get_pid_from_urn($urn);
  $fedora_object = islandora_object_load($pid);
  
  $annotation = $fedora_object['OAC']->content;
  $xml = simplexml_load_string($annotation);
  
   //indentify URN of body to retrieve chars node to hold new annotation content
  $body_urn_array = $xml->xpath("//oa:hasBody");
  $body_urn = (string) $body_urn_array[0]->attributes('rdf', 'about');
  $content_node = $xml->xpath("//rdf:Description[@rdf:about = '$body_urn']//cnt:chars");
  
  $constrained_urn_array = $xml->xpath("//oa:constrainedBy"); 
  $constrained_by_urn = $body_urn = (string) $constrained_urn_array[0]->attributes('rdf', 'resource');
  $target_node = $xml->xpath("//rdf:Description[@rdf:about = '$constrained_by_urn']//cnt:chars");
  $target_value = (string)$target_node[0];
  
  //if color picking is allowed, update here
  if (!$optimized = variable_get('islandora_annotation_optimized', TRUE)) {
    $rels = $fedora_object->relationships->get(ISLANDORA_RELS_EXT_URI, 'hasColor');
    $old_color = $rels[0]['object']['value'];
    if (!empty($new_color) && $old_color != $new_color) {
      $fedora_object->relationships->remove(ISLANDORA_RELS_EXT_URI, 'hasColor', NULL, TRUE);
      $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, 'hasColor', $new_color, TRUE);
      $changed = TRUE;
    }
  }
  
   $rels = $fedora_object->relationships->get(ISLANDORA_RELS_EXT_URI, 'strokeWidth');
    $old_stroke_width = $rels[0]['object']['value'];
    if ($old_stroke_width != $new_stroke_width) {
      $fedora_object->relationships->remove(ISLANDORA_RELS_EXT_URI, 'strokeWidth', NULL, TRUE);
      $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, 'strokeWidth', $new_stroke_width, TRUE);
      $svg_xml = simplexml_load_string($target_value);
      $svg_xml['stroke-width'] = $new_stroke_width.'%';
      $target_node[0]->{0} = str_replace('<?xml version="1.0"?>','',$svg_xml->asXML());
     $test_text = urlencode((string)$target_node[0]);
      $changed = TRUE;
    }

  //update the rels if the category has changed
  $anno_type = $fedora_object->relationships->get(ISLANDORA_RELS_EXT_URI, 'isAnnotationType');
  $anno_type_value = $annot_type['object']['value'];
  if ($type != $anno_type_value) {
    $fedora_object->relationships->remove(ISLANDORA_RELS_EXT_URI, 'isAnnotationType', NULL, TRUE);
    $fedora_object->relationships->add(ISLANDORA_RELS_EXT_URI, 'isAnnotationType', $type, TRUE);
  }
  

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
  
  if ((string) $content_node[0][0] != $content) {
    $content_node[0][0] = $content;
    $changed = TRUE;
  }


  // persist updated annotation
  if ($changed) {
    $test_xml = $xml->asXML();
    $fedora_object['OAC']->content = $xml->asXML();
  }
}

function islandora_annotations_get_type_terms() {
  $type_field = variable_get('annotation_type_search_field', 'anno_category_s');
  $solr = variable_get('islandora_solr_url', 'localhost:8080/solr');
  $url = "http://$solr/select?indent=on&version=2.2&q=rels.hasModel:islandora\:OACCModel&fl=$type_field&fq=&start=0&rows=10000&fl=PID&qt=standard&wt=json";
  $results = json_decode(file_get_contents($url), TRUE);
  $docs = $results['response']['docs'];

  foreach ($docs as $doc) {
    $terms[$doc[$type_field][0]] = 1;
  }
  return array_keys($terms);
}

/**
 * return each solr doc found
 * 
 * @type string
 *   the mads genre to search for
 * @return string
 *   json string
 */
function islandora_annotations_get_title_terms($type) {
  $title_field = variable_get('annotation_title_search_field', 'mads_titles_sv');
  $solr = variable_get('islandora_solr_url', 'localhost:8080/solr');
  $url = "http://$solr/select/?q=mads_genre_it:$type&version=2.2&start=0&rows=1000&indent=on&fl=$title_field,PID&wt=json&sort=$title_field%20asc";
  $results = json_decode(file_get_contents($url), TRUE);
  $docs = $results['response']['docs'];
  //asort($docs);
  echo json_encode($docs);
  exit();
}

function islandora_annotations_get_mads_as_json($pid) {
  $object = islandora_object_load($pid);
  if (!isset($object) || !isset($object['MADS'])) {
    echo 'error loading MADS datastream';
    exit();
  }
  $mads = $object['MADS']->content;
  $doc = new DomDocument();
  $doc->loadXML($mads);
  $elements = $doc->getElementsByTagName('related');
  $arr = array();
  $str = "";
  foreach ($elements as $element) {
    $attr = $element->getAttribute('otherType');
    if (!empty($attr)) {
      $str = ucfirst(trim($attr));
      $str = str_replace('_', ' ', $str);
      $arr[$str] = trim($element->nodeValue);
      //$str .= $attr .': '. trim($element->nodeValue). '<br />';
    }
  }
  echo json_encode($arr);
  //echo $str;
  exit();
}