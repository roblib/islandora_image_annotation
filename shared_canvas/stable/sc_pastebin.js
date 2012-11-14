
function init_pb() {
  $('#create_body').append('<li id="pasteBinSel">&nbsp; Public: <span style="float:right"><input type="radio" name="blog_radio" checked = "checked" id="pb_pastebin"></span></span></li>');
  $("#create_body li:even").addClass("alt").hide();
}

//add annotation to fedora

function pb_postData(title, data, type, color) {
  
  data = encodeURI(data);
  $.ajax({
    type:'POST',
    async:false,
    url:islandora_canvas_params.islandora_post_url,
    data: {
      title:title,
      data:data,
      type:type,
      color:color
    },
    success: function(data,status,xhr) {
      pb_getPaste(data);
    },
    error: function(data,status,xhr) {
      alert('Failed to post')
    }
  });

}

// Adds divs for each type
//

function pb_getList() {
  islandora_canvas_params.mappings = new Array();
  $.ajax({
    type:'GET',
    async:false,
    url: islandora_canvas_params.get_annotation_list_url,
    success: function(data,status,xhr) {
      var listdata = $.parseJSON(data);
      var pids = listdata.pids;

      if( pids != null){
        for (var i=0,info;i < pids.length;i++){
          islandora_canvas_params.mappings[pids[i]['urn']] = pids[i]['color']

          info=pids[i]['id'];
          var pid = info;
          var temp = pids[i]['type'];
          var fixed_cat = temp.replace(/[^\w]/g,'');
          if(temp != type){

            var type_class = "annoType_" + fixed_cat;
            var id = 'islandora_annoType_'+ fixed_cat;
            var idSelector = '#' + id;
    
            if($(idSelector).length == 0){
              header = '<div  class = "islandora_comment_type" id = "'+ id + '"><div class = "islandora_comment_type_title">' + temp + '</div></div>';
              $('#comment_annos_block').append(header);
            }
          }

          $('#canvases .canvas').each(function() {
            var cnv = $(this).attr('canvas');
            pb_getPaste(pid);
          });
          var type = temp;
        }
      }

      $(".islandora_comment_type_title").off();

      //changed from simple toggle to allow for out of sync elements

      $(".islandora_comment_type_title").ready().on("click", function(){
        if($(this).siblings('.canvas_annotation').is(":visible")){
          $(this).siblings('.canvas_annotation').hide();
        } else{
          $(this).siblings('.canvas_annotation').show();
        }
      });
      
    },
    error: function(data,status,xhr) {
    // alert('Failed to retrieve List')
    }

  });
 
}


// get annotation data from Fedora and send it to load_comment_anno to be displayed

function pb_getPaste(pid) {

  $.ajax({
    type:'GET',
    url: islandora_canvas_params.islandora_get_annotation + pid,
    success: function(data,status,xhr) {
   
      load_commentAnno(data);
    },
    error: function(data,status,xhr) {
    
    }
  });
}


function pb_deleteAnno(urn) {

  var selector = '#anno_'+urn;
  var classSelector = '.svg_'+urn;
  $.ajax({
    type:'POST',
    url:islandora_canvas_params.islandora_delete_annotation + urn,
    data: urn,
    success: function(data,status,xhr) {
      $(selector).next().remove();
      $(selector).remove();
      $(classSelector).remove();
    },
    error: function(data,status,xhr) {
    //   alert('Failed to delete annotation')
    }
  });
}


function pb_update_annotation(urn, title,annoType, content, color){
  $.ajax({
    type:'POST',
    url:islandora_canvas_params.islandora_update_annotation,
    data: {
      urn:urn,
      title:title,
      annoType:annoType,
      content:content,
      color:color
    },
    success: function(data,status,xhr) {
      $('#create_annotation_box').hide();
      var selector = '#anno_'+urn;
      var text = $(selector).text().trim().substring(2,100);
      old_title = $(selector).html();
      new_title = old_title.replace(text, title);
      $(selector).html(new_title);

      $(selector).next('.comment_text').find('.comment_type').text(annoType);
      $(selector).next('.comment_text').find('.comment_content').text(content);

    },
    error: function(data,status,xhr) {
      alert('Failed to update annotation')
    }
  });
  $('#create_annotation').empty().append('Annotate');
  $('#create_annotation').css({
    color:'#000000'
  });
}