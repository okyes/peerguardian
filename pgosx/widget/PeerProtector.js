/*
    Copyright 2005,2006 Brian Bergstrand

    This program is free software; you can redistribute it and/or modify it under the terms of the
    GNU General Public License as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along with this program;
    if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

var timer;

function getLocalizedString (key)
{
    try {
        var ret = localizedStrings[key];
        if (ret === undefined)
            ret = key;
        return ret;
    } catch (ex) {}
 
    return key;
}

function stats() {
	if (PPWidgetHelper) {
		var d = PPWidgetHelper.statistics();
		if (d === undefined) {
			alert("PPWidgetHelper.statistics() failed!");
			return;
		}
		document.getElementById("totalAddresses").innerText = d[0];
		document.getElementById("incomingBlocks").innerText = d[1];
		document.getElementById("outgoingBlocks").innerText = d[2];
		document.getElementById("incomingAllows").innerText = d[3];
		document.getElementById("outgoingAllows").innerText = d[4];
		document.getElementById("totalBlocks").innerText = d[5];
		document.getElementById("totalAllows").innerText = d[6];
        document.getElementById("connPerSec").innerText = d[7];
        document.getElementById("blockPerSec").innerText = d[8];
	}  
	else {
		alert("Widget plugin not loaded.");
	}
}

function setup() {
	document.getElementById('totalText').innerText = getLocalizedString('Blocked Addresses');
	document.getElementById('inBlkText').innerText = getLocalizedString('Incoming Blocks');
	document.getElementById('outBlkText').innerText = getLocalizedString('Outgoing Blocks');
	document.getElementById('totalBlkText').innerText = getLocalizedString('Total Blocks');
	document.getElementById('inAllowText').innerText = getLocalizedString('Incoming Allows');
	document.getElementById('outAllowText').innerText = getLocalizedString('Outgoing Allows');
	document.getElementById('totalAllowText').innerText = getLocalizedString('Total Allows');
    document.getElementById('connPerSecText').innerText = getLocalizedString('Connections / sec');
    document.getElementById('blockPerSecText').innerText = getLocalizedString('Blocks / sec');
	
	if (window.widget) {
		widget.onshow = onshow;
		widget.onhide = onhide;
	}
	
}

function onshow () {
    if (timer == null) {
        stats();
        timer = setInterval("stats();", "1000");
    }
}

function onhide () {
    if (timer != null) {
        clearInterval(timer);
        timer = null;
    }
}

