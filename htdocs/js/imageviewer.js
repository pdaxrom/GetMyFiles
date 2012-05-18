Imageviewer = function() {
	function isImageFile(file) {
	    return ((file.indexOf(".jpg", file.length - 4) > 0) ||
		    (file.indexOf(".gif", file.length - 4) > 0) ||
		    (file.indexOf(".png", file.length - 4) > 0));
	}
    return {
	large:function(obj) {
	    var imgbox = document.getElementById("imgbox");
	    imgbox.style.visibility = 'visible';
	    var img = document.createElement("img");
	    img.src = obj.href;

	    img.onload = function() {
		if(img.addEventListener){
		    img.addEventListener('mouseout', Imageviewer.out, false);
		} else {
		    img.attachEvent('onmouseout', Imageviewer.out);
		}

		imgbox.innerHTML = '';
		imgbox.appendChild(img);

		scale = 1;
		if (img.height > window.innerHeight) {
		    img.style.height = (window.innerHeight) + 'px';
		    scale = window.innerHeight / img.height;
		}

		imgbox.style.left = (window.innerWidth - img.width * scale) / 2 + 'px';
	    }
	},
	out:function() {
	    document.getElementById("imgbox").style.visibility = 'hidden';
	    document.getElementById("imgbox").removeChild((document.getElementById("imgbox").lastChild));
	},
	init:function() {
	    var e = document.createElement("div");
	    e.setAttribute("id", "imgbox");
	    e.setAttribute("style", "position:fixed; z-index:100; outline:none; cursor:pointer; visibility: hidden; overflow: hidden; margin: 0 auto;");
	    document.body.insertBefore(e, document.body.childNodes[0]);
	    var links = document.getElementsByTagName("a");
	    for (var i = 0; i < links.length; i++) {
		var url = links[i].getAttribute("href", 2);
		if (isImageFile(url)) {
		    links[i].setAttribute("onmouseout", "Imageviewer.out(this)");
		    links[i].setAttribute("onmouseover", "Imageviewer.large(this)");
		}
	    }
	}
    };
}();
