
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
<legend>FPP TPLink Config</legend>

<script>

var tplinkConfig = <? echo json_encode($pluginJson, JSON_PRETTY_PRINT); ?>;


var uniqueId = 1;
var modelOptions = "";
function AddCURLItem() {
    var id = $("#tplinkTableBody > tr").length + 1;
    var html = "<tr class='fppTableRow";
    if (id % 2 != 0) {
        html += " oddRow'";
    }
    html += "'><td class='colNumber rowNumber'>" + id + ".</td><td><span style='display: none;' class='uniqueId'>" + uniqueId + "</span></td>";
    html += "<td><input type='text' minlength='7' maxlength='15' size='15' pattern='^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$' class='ipaddress' /></td>";
    html += "<td><input type='number' value='1' min='1' max='10000000' class='startchan' />"
    html += "</tr>";
    
    $("#tplinkTableBody").append(html);

    newRow = $('#tplinkTableBody > tr').last();
    $('#tplinkTableBody > tr').removeClass('selectedEntry');
    DisableButtonClass('deleteEventButton');
    uniqueId++;

    return newRow;
}

function SaveCURLItem(row) {
    var ip = $(row).find('.ipaddress').val();
    var startchan = parseInt($(row).find('.startchan').val(),10);

    var json = {
        "ip": ip,
        "startchannel": startchan
    };
    return json;
}

function SaveCURLItems() {
    var tplinkConfig = [];
    var i = 0;
    $("#tplinkTableBody > tr").each(function() {
        tplinkConfig[i++] = SaveCURLItem(this);
    });
    
    var data = JSON.stringify(tplinkConfig);
    $.ajax({
        type: "POST",
        url: 'fppjson.php?command=setPluginJSON&plugin=tplink',
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
function RemoveCURLItem() {
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
        <input type="button" value="Save" class="buttons genericButton" onclick="SaveCURLItems();">
        <input type="button" value="Add" class="buttons genericButton" onclick="AddCURLItem();">
        <input id="delButton" type="button" value="Delete" class="deleteEventButton disableButtons genericButton" onclick="RemoveCURLItem();">
    </td>
</tr>
</table>

<div class='fppTableWrapper fppTableWrapperAsTable'>
<div class='fppTableContents'>
<table class="fppTable" id="tplinkTable"  width='100%'>
<thead><tr class="fppTableHeader"><th>#</th><th></th><th>IP</th><th>Start Chan</th></tr></thead>
<tbody id='tplinkTableBody'>
</tbody>
</table>
</div>

</div>
<div>
<p>
<p>Use cURL to sent {Message} to 'http://{IP}:{Port}{URL}'. 
Leave {Message} blank for GET requests. 
<p>
</div>
</div>
<script>

$.each(tplinkConfig, function( key, val ) {
    var row = AddCURLItem();
    $(row).find('.ipaddress').val(val["ip"]);
    $(row).find('.startchannel').val(val["startchannel"]);

});
</script>

</fieldset>
</div>
