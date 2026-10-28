
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

require_once __DIR__ . "/tapo_common.php";

// The Tapo account is kept only as auth hashes (see tapo_common.php). Convert
// a plaintext password left by an earlier version: a plugin-wide TapoPassword,
// or the per-entry account older versions stored on each device row.
global $pluginSettings;
$tapoEmail = isset($pluginSettings['TapoUsername']) ? $pluginSettings['TapoUsername'] : "";
$tapoHaveHash = isset($pluginSettings['TapoAuthV2']) && $pluginSettings['TapoAuthV2'] != "";
if (isset($pluginSettings['TapoPassword']) && $pluginSettings['TapoPassword'] != "") {
    if (!$tapoHaveHash) {
        tapoSaveAccount($tapoEmail, $pluginSettings['TapoPassword']);
        $tapoHaveHash = true;
    } else {
        tapoForgetSetting("TapoPassword");
    }
}
if (!$tapoHaveHash) {
    foreach ($pluginJson as $entry) {
        $u = returnIfExists($entry, "username");
        if ($u != "") {
            tapoSaveAccount($u, returnIfExists($entry, "password"));
            $tapoEmail = $u;
            $tapoHaveHash = true;
            break;
        }
    }
}
tapoStripEntryCredentials($settings['configDirectory'] . "/plugin.tplink.json");
$pluginJson = convertAndGetSettings();
?>

<div id="tapoAccount" class="settings">
<fieldset>
<legend>Kasa/Tapo Account</legend>
<p>Tapo devices, and Kasa devices whose firmware has moved to TP-Link's newer KLAP protocol, log in with this account: the TP-Link ID email and password you sign in to the Kasa or Tapo app with (both are case-sensitive). Kasa devices on the older protocol don't need it. Newer Tapo firmware also needs Third-Party Compatibility turned on in the Tapo app (Me &rarr; Third-Party Services).</p>
<p>The password is not stored. Saving keeps only a hash of the email and password that the devices check, which lets FPP control them but cannot be turned back into your password.</p>
<table border=0>
<tr><td>Email:</td><td><input type='text' id='tapoEmail' size='32' maxlength='128' autocomplete='username' value='<? echo htmlspecialchars($tapoEmail, ENT_QUOTES, 'UTF-8'); ?>' /></td></tr>
<tr><td>Password:</td><td><input type='password' id='tapoPassword' size='32' maxlength='128' autocomplete='new-password' placeholder='<? echo $tapoHaveHash ? "Saved (enter to change)" : "Not set"; ?>' /></td></tr>
<tr><td></td><td><input type='button' class='buttons genericButton' value='Save Kasa/Tapo Account' onclick='SaveTapoAccount();' /> <span id='tapoAccountStatus'></span></td></tr>
</table>
<script>
function SaveTapoAccount() {
    var email = $('#tapoEmail').val().trim();
    var password = $('#tapoPassword').val();
    if (email == '' || password == '') {
        $('#tapoAccountStatus').text('Enter both the email and password (the password is needed to make the hash).');
        return;
    }
    $('#tapoAccountStatus').text('Saving...');
    $.post('plugin.php?plugin=fpp-plugin-tplink&page=tapo_account.php&nopage=1',
           { email: email, password: password })
        .done(function () {
            $('#tapoPassword').val('').attr('placeholder', 'Saved (enter to change)');
            $('#tapoAccountStatus').text('Saved.');
            SetRestartFlag(2);
        })
        .fail(function (xhr) {
            var msg = 'Save failed.';
            try { msg = JSON.parse(xhr.responseText).error || msg; } catch (e) {}
            $('#tapoAccountStatus').text(msg);
        });
}
</script>
</fieldset>
</div>


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
    html += ">Tasmota Switch</option><option value='tapolight'";
    if(type == 'tapolight') {html += " selected ";}
    html += ">Tapo Light</option><option value='taposwitch'";
    if(type == 'taposwitch') {html += " selected ";}
    html += ">Tapo Switch</option></select>";
    var plugDisplay = (type == 'switch' || type == 'tasmotaswitch' || type == 'taposwitch') ? '' : 'display: none;';
    html += "<td><input type='number' value='0' min='0' max='10' class='plugnum' style='" + plugDisplay + "' />";
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

    $('#tplinkTableBody').on('change', '.devicetype', function() {
        var row = $(this).closest('tr');
        var val = $(this).val();
        var isPlug = (val == 'switch' || val == 'tasmotaswitch' || val == 'taposwitch');
        row.find('.plugnum').toggle(isPlug);
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
<p><b>Plugs and switches in a sequence:</b> a start channel means the sequence owns the plug. When a playlist starts, the plug is sent the channel's state on the first frame. After that the plugin sends only when the state changes (127 or more is on, below 127 is off). Leave the start channel at 0 for plugs you control with commands (playlist lead-ins, Home Assistant).</p>
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
