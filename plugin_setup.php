
<?
function returnIfExists($json, $setting) {
    if ($json == null) {
        return "";
    }
    if (array_key_exists($setting, $json)) {
        return $json[$setting];
    }
    return "";
}

function convertAndGetSettings() {
    global $settings;
        
    $cfgFile = $settings['configDirectory'] . "/plugin.tplink.json";
    if (file_exists($cfgFile)) {
        $j = file_get_contents($cfgFile);
        $json = json_decode($j, true);
        return $json;
    }
    $j = "[]";
    return json_decode($j, true);
}

$pluginJson = convertAndGetSettings();
?>


<div id="global" class="settings">
<fieldset>
<legend>FPP TP-Link Config</legend>

<script>

var tplinkConfig = <? echo json_encode($pluginJson, JSON_PRETTY_PRINT); ?>;


var uniqueId = 1;
var modelOptions = "";
function AddTPLinkItem(type) {
    var id = $("#tplinkTableBody > tr").length + 1;
    var html = "<tr class='fppTableRow";
    if (id % 2 != 0) {
        html += " oddRow'";
    }
    html += "'><td class='colNumber rowNumber'>" + id + ".</td><td><span style='display: none;' class='uniqueId'>" + uniqueId + "</span></td>";
    html += "<td><input type='text' minlength='7' maxlength='15' size='15' pattern='^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$' class='ipaddress' /></td>";
    html += "<td><input type='number' value='0' min='0' max='10000000' class='startchan' />";
    html += "<td><select class='devicetype'>";
    html += "<option value='light'";
    if(type == 'light') {html += " selected ";}
    html += ">TPLink Light</option><option value='switch'";
    if(type == 'switch') {html += " selected ";}
    html += ">TPLink Switch/Plug</option><option value='goveelight'";
    if(type == 'goveelight') {html += " selected ";}
    html += ">Govee Light</option><option value='tasmotalight'";
    if(type == 'tasmotalight') {html += " selected ";}
    html += ">Tasmota Light</option><option value='tasmotaswitch'";
    if(type == 'tasmotaswitch') {html += " selected ";}
    html += ">Tasmota Switch</option></select>";
    html += "<td><input type='number' value='0' min='0' max='10' class='plugnum' />";
    html += "</tr>";
    //selected
    $("#tplinkTableBody").append(html);

    newRow = $('#tplinkTableBody > tr').last();
    $('#tplinkTableBody > tr').removeClass('selectedEntry');
    DisableButtonClass('deleteEventButton');
    uniqueId++;

    return newRow;
}

function SaveTPLinkItem(row) {
    var ip = $(row).find('.ipaddress').val();
    var startchan = parseInt($(row).find('.startchan').val(),10);

    var plugnum = parseInt($(row).find('.plugnum').val(),10);
	var devicetype = $(row).find('.devicetype').val();

    var json = {
        "ip": ip,
        "startchannel": startchan,
        "devicetype": devicetype,
        "plugnumber": plugnum
    };
    return json;
}

function SaveTPLinkItems() {
    var tplinkConfig = [];
    var i = 0;
    $("#tplinkTableBody > tr").each(function() {
        tplinkConfig[i++] = SaveTPLinkItem(this);
    });
    
    var data = JSON.stringify(tplinkConfig);
    $.ajax({
        type: "POST",
	url: 'api/configfile/plugin.tplink.json',
        dataType: 'json',
        async: false,
        data: data,
        processData: false,
        contentType: 'application/json',
        success: function (data) {
           SetRestartFlag(2);
        }
    });
}


function RenumberRows() {
    var id = 1;
    $('#tplinkTableBody > tr').each(function() {
        $(this).find('.rowNumber').html('' + id++ + '.');
        $(this).removeClass('oddRow');

        if (id % 2 != 0) {
            $(this).addClass('oddRow');
        }
    });
}
function RemoveTPLinkItem() {
    if ($('#tplinkTableBody').find('.selectedEntry').length) {
        $('#tplinkTableBody').find('.selectedEntry').remove();
        RenumberRows();
    }
    DisableButtonClass('deleteEventButton');
}


$(document).ready(function() {
                  
    $('#tplinkTableBody').sortable({
        update: function(event, ui) {
            RenumberRows();
        },
        item: '> tr',
        scroll: true
    }).disableSelection();

    $('#tplinkTableBody').on('mousedown', 'tr', function(event,ui){
        $('#tplinkTableBody tr').removeClass('selectedEntry');
        $(this).addClass('selectedEntry');
        EnableButtonClass('deleteEventButton');
    });
});

</script>
<div>
<table border=0>
<tr><td colspan='2'>
        <input type="button" value="Save" class="buttons genericButton" onclick="SaveTPLinkItems();">
        <input type="button" value="Add" class="buttons genericButton" onclick="AddTPLinkItem('light');">
        <input id="delButton" type="button" value="Delete" class="deleteEventButton disableButtons genericButton" onclick="RemoveTPLinkItem();">
    </td>
</tr>
</table>

<div class='fppTableWrapper fppTableWrapperAsTable'>
<div class='fppTableContents'>
<table class="fppSelectableRowTable" id="tplinkTable"  width='100%'>
<thead><tr class="fppTableHeader"><th>#</th><th></th><th>IP</th><th>Start Chan</th><th>Device Type</th><th>Plug ID</th></tr></thead>
<tbody id='tplinkTableBody'>
</tbody>
</table>
</div>

</div>
<div>
<p>
<p>
</div>
</div>
<script>

$.each(tplinkConfig, function( key, val ) {
    var row = AddTPLinkItem(val["devicetype"]);
    $(row).find('.ipaddress').val(val["ip"]);
    $(row).find('.startchan').val(val["startchannel"]);
    $(row).find('.plugnum').val(val["plugnumber"]);
});
</script>

</fieldset>
</div>
