<?php

include "template.class.php";

function is_defined($var) {
    if (isset($_REQUEST[$var])) {
	return true;
    } else {
	return false;
    }
}

$page = "";
if (is_defined("p")) {
    $page = $_REQUEST["p"];
}

if ($page == "") {
    $page = "index";
}

$tmpl_file = "tmpl/".$page.".html";
$lang_file = "lang/ru/".$page.".lng";

if (!file_exists($tmpl_file) || !file_exists($lang_file)) {
    $page = "index";
    $tmpl_file = "tmpl/".$page.".html";
    $lang_file = "lang/ru/".$page.".lng";
}

include $lang_file;

$template = new Template;
$template->load($tmpl_file);

foreach ($txt as $k => $v) {
    $template->replace($k, $v);
}

/*
$template->replace("title", "My Template Class");
$template->replace("name", "William");
$template->replace("datetime", date("m/d/y"));
 */

$template->publish();
?>
