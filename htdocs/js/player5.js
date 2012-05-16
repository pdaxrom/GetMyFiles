var links;
var player;
var html5player = true;
var included_files = new Array();

function include_inhead(url) {
    var html_doc = document.getElementsByTagName('head').item(0);
    var js = document.createElement('script');

    js.setAttribute('language', 'javascript');
    js.setAttribute('type', 'text/javascript');
    js.setAttribute('src', url);
    html_doc.appendChild(js);

    return false;
}

function in_array(needle, haystack) {
    for (var i = 0; i < haystack.length; i++) {
	if (haystack[i] == needle) {
	    return true;
	}
    }
    return false;
}

function require_once(url) {
    if (!in_array(url, included_files)) {
	included_files[included_files.length] = url;
	include_inhead(url);
    }
}

function startPlay(e) {
    //console.log(e.getAttribute("href"));
    if (html5player) {
	player.src = e.getAttribute("href");
	player.play();
    } else {
	player.jPlayer("setMedia", {
		mp3: e.getAttribute("href")
	});
	player.jPlayer("play");
    }
}

function nextPlay() {
    for (var i = 0; i < links.length; i++) {
	if (links[i].classList.contains("selected")) {
	    links[i].classList.remove("selected");
	    links[i].previousSibling.setAttribute("src", "/pics/play.png");
	    while (++i < links.length) {
		if (isAudioFile(links[i].getAttribute("href", 2))) {
		    links[i].previousSibling.setAttribute("src", "/pics/stop.png");
		    selectAndPlay(links[i]);
		    break;
		}
	    }
	    break;
	}
    }
}

function stopPlay() {
    if (html5player) {
	player.pause();
    } else {
	player.jPlayer("stop");
    }
}

function isAudioFile(file) {
    return ((file.indexOf(".mp3", file.length - 4) > 0) ||
	    (file.indexOf(".ogg", file.length - 4) > 0));
}

function selectAndPlay(e) {
    e.classList.add("selected");
    startPlay(e);
}

function init() {
    if (html5player) {
	player = document.createElement("audio");
	player.addEventListener("ended", nextPlay);
    } else {
	var elem = document.createElement("div");
	elem.id = "jquery_jplayer";
	document.body.insertBefore(elem,document.body.childNodes[0]);
	player = $("#jquery_jplayer");
	player.jPlayer({
	    ready: function () {
	    //	playNext;
	    },
	    timeupdate: function(event) {
	    //	my_extraPlayInfo.text(parseInt(event.jPlayer.status.currentPercentAbsolute, 10) + "%");
	    },
	    play: function(event) {
	    //	my_playState.text(opt_text_playing);
	    },
	    pause: function(event) {
	    //	my_playState.text(opt_text_selected);
	    },
	    ended: function(event) {
	    //	my_playState.text(opt_text_selected);
		nextPlay();
	    },
	    swfPath: "/js",
	    cssSelectorAncestor: "#jp_container",
	    supplied: "mp3",
	    wmode: "window"
	});
    }
    links = document.getElementsByTagName("a");
    for (var i = 0; i < links.length; i++) {
	var url = links[i].getAttribute("href", 2);
	if (isAudioFile(url)) {
	    var new_element = document.createElement('img');
	    new_element.setAttribute("src", "/pics/play.png");
	    links[i].parentNode.insertBefore(new_element, links[i]);
	    new_element.onclick = function() {
		var selected = document.querySelector(".selected");
		if (selected) {
		    selected.classList.remove("selected");
		    selected.previousSibling.setAttribute("src", "/pics/play.png");
		}
		if (this.nextSibling == selected) {
		    stopPlay();
		} else {
		    this.setAttribute("src", "/pics/stop.png");
		    selectAndPlay(this.nextSibling);
		}
		return false;
	    }
	}
    }
}

function detectBrowser()
{
    if (navigator.userAgent.indexOf("Opera") >= 0) {
	html5player = false;
    } else if ((navigator.userAgent.indexOf("KHTML") >= 0) &&
	    (navigator.userAgent.indexOf("like Gecko") >= 0) &&
	    (navigator.userAgent.indexOf("WebKit") >= 0)) {
	html5player = true;
    } else if ((navigator.userAgent.indexOf("Mozilla") >= 0) &&
	    (navigator.userAgent.indexOf("Gecko") >= 0)) {
	html5player = false;
    } else if ((navigator.userAgent.indexOf("MSIE") >= 0) &&
	    (navigator.userAgent.indexOf("Trident") >= 0)) {
	html5player = false;
    }
    if (!html5player) {
	require_once("/js/jquery-and-jplayer.min.js");
    }
}

window.onload = init;

detectBrowser();
