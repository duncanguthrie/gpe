<?php

function menu_entry($page, $title, $link,$thispage)
{
  if ($page == $thispage) {
    echo "<b>$title</b>";
  } else {
    echo "<a href='$link'>$title</a>";
  }
}

?>

<div class="menu">
<small>

<?php menu_entry("home", "Home", "/", $pagename); ?>
 |
<?php menu_entry("projects", "Projects", "/projects/", $pagename); ?>
 |
<?php menu_entry("documentation", "Documentation", "/documentation.phtml", $pagename); ?>
 |
<?php menu_entry("screenshots", "Screenshots", "/screenshots/", $pagename); ?>
 |
<?php menu_entry("download", "Download", "/download.phtml", $pagename); ?>
 |
<?php menu_entry("contact", "Mailing list", "/contact.phtml", $pagename); ?>
 |
<?php menu_entry("screenshots", "Screenshots", "/screenshots/", $pagename); ?>
 |
<A href="http://handhelds.org/bugzilla/">Bugs</A>
 |
<?php menu_entry("cvs", "CVS", "/cvs.phtml", $pagename); ?>
 |
<?php menu_entry("linkres", "Links/Resources", "/linkres.phtml", $pagename); ?>

</small>
</div>
