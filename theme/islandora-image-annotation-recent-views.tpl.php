<div class="islandora-anno-recent-views">
   <dl class ="islandora-anno-recent-views">
  <?php foreach($variables['items'] as $item): ?> 
    <dt class="islandora-anno-recent-views-link"><?php print $item['link']; ?></dt>
    <dd class="islandora-anno-recent-views-tn">
      <a href="<?php print $item['url_only']?>">"<?php print $item['TN']; ?></a> 
    </dd>  
   <?php endforeach; ?>
    </dl>
</div>