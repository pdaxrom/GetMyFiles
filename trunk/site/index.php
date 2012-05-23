<?php

include "build_version.inc";

setcookie ('PHPSESSID', $_COOKIE['PHPSESSID'], time() + 60 * 30, '/');

session_start();

include "template.class.php";

function is_defined($var) {
    if (isset($_REQUEST[$var])) {
	return true;
    } else {
	return false;
    }
}

function is_session($var) {
    if (isset($_SESSION[$var])) {
	return true;
    } else {
	return false;
    }
}

$lang = "en";
if (is_session("lang")) {
    $lang = $_SESSION["lang"];
}

if (is_defined("l")) {
    $l = $_REQUEST["l"];
    if ($lang == "ru" || $lang == "en") {
	$lang = $l;
	$_SESSION["lang"] = $lang;
    }
}

$new_lang = "Русский";
$nlang = "";
if ($lang == "ru") {
    $new_lang = "English";
    $nlang = "en";
} else {
    $new_lang = "Русский";
    $nlang = "ru";
}

$page = "";
if (is_defined("p")) {
    $page = $_REQUEST["p"];
}

if ($page == "") {
    $page = "index";
}

$tmpl_file = "tmpl/".$page.".html";
$lang_file = "lang/".$lang."/".$page.".lng";

if (!file_exists($tmpl_file)) {
    $page = "index";
    $tmpl_file = "tmpl/".$page.".html";
}

if (!file_exists($lang_file)) {
    $lang = "en";
    $lang_file = "lang/en/".$page.".lng";
}

include $lang_file;

$template = new Template;
$template->load($tmpl_file);

foreach ($txt as $k => $v) {
    $template->replace($k, $v);
}

$template->replace("lang_link", "?p=".$page."&l=".$nlang);
$template->replace("new_lang", $new_lang);
$template->replace("version", $build_version);
/*
$template->replace("title", "My Template Class");
$template->replace("name", "William");
$template->replace("datetime", date("m/d/y"));
 */

$template->publish();
?>
