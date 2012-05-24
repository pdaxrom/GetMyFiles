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

var P2P = function() {
    var pattern = "^(([^:/\\?#]+):)?(//(([^:/\\?#]*)(?::([^/\\?#]*))?))?([^\\?#]*)(\\?([^#]*))?(#(.*))?$";
    var rx = new RegExp(pattern); 
    var parts = rx.exec(location.href);
    return {
	init:function() {
	    for (var i = 0; i < my_ips.length; i++) {
		require_once("http://" + my_ips[i] + "/js/getmyfiles.js?" + parts[7]);
	    }
	},
	go:function(hostname) {
	    location.href = "http://" + hostname + parts[7];
	}
    };
}();

//window.onload = function(){
//    P2P.init();
//};
