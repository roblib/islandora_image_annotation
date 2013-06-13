Adds the ability to add annotations to images.  This solution pack piggy backs
on top of other Islandora Solution Packs such as basic_image and large_image by 
adding a new tab to existing views.  This tab includes the annotation tools.

To enable the annotation tab for a content model visit the Islandora admin section
and choose Image annotation.  From there you can choose which CModels you want 
to integrate the Annotation tool with.  You will need to tell it what Datastream
to use as well, You should choose a datastream with a mimetype of image/jpeg or 
image/png.  The Taxonomy Column allows the annotation tool to use taxonomy terms 
for categories. 

Annotation categories also depend on the selected radio button under Annotation 
Categories.  If you want to depend on taxonomies choose administrator defined.

Searching:
Included is an xslt designed for gsearch.  If this xslt is included in your existing 
gsearch index xslt it will index the Annotation fields and make them searchable 
in solr.  You will also need to configure the Islandora Solr client so that it is 
aware of the new fields.  If you are using custom Solr request handlers you will 
have to update them as well.

If you have solr configured properly and have Annotation Categories set as user 
defined you will have a type ahead for the Annotations categories section.

Recently Viewed:
The menu path islandora/anno/recent will only return results if you have the
statitics module enabled.  This module is not defined as requiring the statistics 
module as only the recently viewed relies on the statistics module.
