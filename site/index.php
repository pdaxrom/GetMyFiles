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

$tmpl_head_file = "tmpl/header.html";
$tmpl_foot_file = "tmpl/footer.html";

$head_file = "lang/".$lang."/header.lng";
$foot_file = "lang/".$lang."/footer.lng";

include $head_file;
include $lang_file;
include $foot_file;

$template = new Template;
$template->load($tmpl_file);

$templ_header = new Template;
$templ_header->load($tmpl_head_file);

$templ_footer = new Template;
$templ_footer->load($tmpl_foot_file);

foreach ($txt as $k => $v) {
    $template->replace($k, $v);
    $templ_header->replace($k, $v);
    $templ_footer->replace($k, $v);
}

foreach ($head_txt as $k => $v) {
    $template->replace($k, $v);
    $templ_header->replace($k, $v);
    $templ_footer->replace($k, $v);
}

foreach ($foot_txt as $k => $v) {
    $template->replace($k, $v);
    $templ_header->replace($k, $v);
    $templ_footer->replace($k, $v);
}

$template->replace("lang_link", "?p=".$page."&l=".$nlang);
$template->replace("new_lang", $new_lang);
$template->replace("version", $build_version);

$templ_header->replace("lang_link", "?p=".$page."&l=".$nlang);
$templ_header->replace("new_lang", $new_lang);
$templ_header->replace("version", $build_version);

$templ_footer->replace("lang_link", "?p=".$page."&l=".$nlang);
$templ_footer->replace("new_lang", $new_lang);
$templ_footer->replace("version", $build_version);


$templ_header->publish();
$template->publish();
$templ_footer->publish();
?>
