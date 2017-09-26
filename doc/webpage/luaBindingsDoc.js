var classSelection = "1";
var memberSelection = "1_1";
var lastNavTable = {};

function showClassNav( id) {
	var navid = 'nav_' + id;
	var navlistid = 'list_' + id;
	var titleid = 'title_' + id;
	var descriptionid = 'description_' + id;
	var allElements = document.getElementsByTagName("*");
	for(var i=0; i < allElements.length; i++)
	{
		if (allElements[i].className === "navclass")
		{
			if (allElements[i].id === navid) {
				allElements[i].style.background = "#e8f8e8";
				allElements[i].style.fontWeight = 'bold';
				allElements[i].style.display = 'block';
			}
			else
			{
				allElements[i].style.fontWeight = 'normal';
				allElements[i].style.background = "#ecffe6";
			}
		}
		else if (allElements[i].className === "navmemberlist")
		{
			if (allElements[i].id === navlistid) {
				allElements[i].style.background = "#e8f8e8";
				allElements[i].style.display = 'block';
			} else {
				allElements[i].style.background = "#ecffe6";
				allElements[i].style.display = 'none';
			}
		}
		else if (allElements[i].className === "nav_title_memberlist")
		{
			if (allElements[i].id === titleid) {
				allElements[i].style.display = 'block';
			} else {
				allElements[i].style.display = 'none';
			}
		}
		else if (allElements[i].className === "classdescription")
		{
			if (allElements[i].id === descriptionid) {
				allElements[i].style.display = 'block';
			} else {
				allElements[i].style.display = 'none';
			}
		}
		else if (allElements[i].className === "memberdescription")
		{
			allElements[i].style.display = 'none';
		}
		else if (allElements[i].className === "navconstructor")
		{
			allElements[i].style.background = "#ecffe6";
		}
		else if (allElements[i].className === "navmethod")
		{
			allElements[i].style.background = "#ecffe6";
		}
	}
	classSelection = id;
}

function showMethod( id) {
	var navid = 'nav_' + id;
	var descriptionid = 'description_' + id;

	var allElements = document.getElementsByTagName("*");
	for(var i=0; i < allElements.length; i++)
	{
		if (allElements[i].className === "navmethod"
		||  allElements[i].className === "navconstructor")
		{
			if (allElements[i].id === navid) {
				allElements[i].style.background = "#e8f8e8";
				allElements[i].style.fontWeight = 'bold';
				allElements[i].style.display = 'block';
			}
			else
			{
				allElements[i].style.fontWeight = 'normal';
				allElements[i].style.background = "#ecffe6";
			}
		}
		else if (allElements[i].className === "memberdescription")
		{
			if (allElements[i].id === descriptionid) {
				allElements[i].style.display = 'block';
			} else {
				allElements[i].style.display = 'none';
			}
		}
	}
	memberSelection = id;
}

function activateClassNav( id) {
	showClassNav( id);
	showMethod( id + "_1");
}

function fillLastNavTable() {
	var allElements = document.getElementsByTagName("*");
	for(var i=0; i < allElements.length; i++)
	{
		if (allElements[i].className === "navmethod" || allElements[i].className === "navclass")
		{
			var idparts = allElements[i].id.split("_");
			if (idparts.length > 2)
			{
				var nav = lastNavTable[ idparts[1]];
				var candidate = parseInt(idparts[2]);
				if (nav == null || nav < candidate) {
					lastNavTable[ idparts[1]] = candidate;
				}
			}
		}
	}
}
function initLoad() {
	activateClassNav( "1");
	fillLastNavTable();
}

function navigateId( id, ofs) {
    if (ofs == 0) return id;
    var idparts = id.split("_");
    var res;
    if (idparts.length == 1) {
        res = (parseInt(idparts[0]) + ofs).toString();
        if (document.getElementById( "nav_" + res) == null) {
            return id;
        } else {
            return res;
        }
    } else {
        var prefix = idparts.slice( 0, idparts.length-1).join("_");
        var newidx = parseInt(idparts[ idparts.length-1]) + ofs;
        res = prefix + "_" + newidx.toString();
        if (document.getElementById( "nav_" + res) != null) {
            return res;
        } else {
            if (ofs > 0) {
                var newprefix = navigateId( prefix, +1);
                if (newprefix == prefix) {
                    return id;
                } else {
                    return newprefix + "_1";
                }
            } else {
                var newprefix = navigateId( prefix, -1);
                var lastnav = lastNavTable[ newprefix];
                if (newprefix == prefix || lastnav == null) {
                    return id;
                } else {
                    return newprefix + "_" + lastnav.toString();
                }
            }
        }
    }
}

document.onkeydown = function(evt) {
    evt = evt || window.event;
    switch (evt.keyCode) {
        case 33://PageUp
            var nav = navigateId( memberSelection, -1);
            var cl = nav.split("_")[0];
            showClassNav( cl);
            showMethod( nav);
            break;
        case 34://PageDown
            var nav = navigateId( memberSelection, +1);
            var cl = nav.split("_")[0];
            showClassNav( cl);
            showMethod( nav);
            break;
    }
}
