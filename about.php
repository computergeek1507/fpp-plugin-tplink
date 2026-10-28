  <div style="margin:0 auto;"> <br />
    <fieldset style="padding: 10px; border: 2px solid #000;">
      <legend>TPLink Plugin Info</legend>
      <div style="overflow: hidden; padding: 10px;">
    <div>
      <div id='credits'>
        <b>TPLink Plugin Developed By:</b><br />
		<br />
        Scott Hanson (computergeek1507)<br />
		<br />
        <a href='https://github.com/computergeek1507/fpp-plugin-tplink'>Git Repository</a><br>
        <a href='https://github.com/computergeek1507/fpp-plugin-tplink/issues'>Bug Reporter</a><br>
		<br />
      </div>
    </div>
    </fieldset>
    <br />
    <fieldset style="padding: 10px; border: 2px solid #000;">
      <legend>TPLink Plugin Discovery</legend>
      <div style="overflow: hidden; padding: 10px;">
<?php
// Discovery broadcasts on the local network and listens for a few seconds, so
// only run it when asked rather than on every visit to this page.
if (isset($_GET['discover'])) {
    require_once __DIR__ . "/discover_common.php";
    if (!function_exists('socket_create')) {
        echo "<p>Discovery needs PHP's sockets extension, which is missing on this system.</p>\n";
    } else {
        $found = tplinkDiscover(3.0);
        if (isset($found['error'])) {
            echo "<p>" . htmlspecialchars($found['error'], ENT_QUOTES, 'UTF-8') . "</p>\n";
        } elseif (count($found) == 0) {
            echo "<p>No devices answered. Discovery only reaches devices on the same network as this FPP; devices on another subnet or VLAN can still be added by IP.</p>\n";
        } else {
            // What is already configured, so those rows say so instead of
            // offering Add again.
            $configured = array();
            $cfgFile = $settings['configDirectory'] . "/plugin.tplink.json";
            $cfg = file_exists($cfgFile) ? json_decode(file_get_contents($cfgFile), true) : array();
            if (is_array($cfg)) {
                foreach ($cfg as $e) {
                    if (is_array($e) && isset($e['ip'])) {
                        $configured[$e['ip'] . '|' . (isset($e['plugnumber']) ? intval($e['plugnumber']) : 0)] = true;
                    }
                }
            }

            echo "<table class='fppSelectableRowTable' style='width:100%'><thead><tr class='fppTableHeader'>";
            foreach (array('IP', 'Name', 'Model', 'Device Type', 'Outlets', 'Protocol', 'MAC', 'Use Plugin Type', '') as $h) {
                echo "<th>" . $h . "</th>";
            }
            echo "</tr></thead><tbody>\n";
            foreach ($found as $d) {
                echo "<tr>";
                foreach (array('ip', 'name', 'model', 'type', 'outlets', 'protocol', 'mac', 'suggest') as $k) {
                    $v = ($k == 'outlets' && !$d[$k]) ? '' : $d[$k];
                    echo "<td>" . htmlspecialchars((string)$v, ENT_QUOTES, 'UTF-8') . "</td>";
                }
                echo "<td>";
                if ($d['code'] != '') {
                    // A strip that reports its outlets gets one entry per
                    // outlet; anything else one entry for the whole device.
                    $plugs = ($d['outlets'] > 0 && $d['code'] == 'switch') ? range(1, $d['outlets']) : array(0);
                    $entries = array();
                    foreach ($plugs as $plug) {
                        if (!isset($configured[$d['ip'] . '|' . $plug])) {
                            $entries[] = array('ip' => $d['ip'], 'startchannel' => 0, 'devicetype' => $d['code'], 'plugnumber' => $plug);
                        }
                    }
                    if (count($entries) == 0) {
                        echo "Added";
                    } else {
                        $label = count($plugs) > 1 ? "Add " . count($entries) . " outlets" : "Add";
                        echo "<input type='button' class='buttons genericButton' value='" . $label . "' data-entries='" .
                             htmlspecialchars(json_encode($entries), ENT_QUOTES, 'UTF-8') . "' onclick='AddDiscoveredDevice(this);' />";
                    }
                }
                echo "</td></tr>\n";
            }
            echo "</tbody></table>\n";
?>
<script>
// Appends the row's entries to plugin.tplink.json (read fresh, so entries
// added meanwhile on the Config page are kept) and saves it, as the Config
// page's Save does.
function AddDiscoveredDevice(button) {
    var entries = JSON.parse($(button).attr('data-entries'));
    $(button).prop('disabled', true).val('Adding...');
    var save = function (config) {
        if (!Array.isArray(config)) {
            config = [];
        }
        entries.forEach(function (e) {
            var dup = config.some(function (c) {
                return c.ip == e.ip && parseInt(c.plugnumber || 0, 10) == e.plugnumber;
            });
            if (!dup) {
                config.push(e);
            }
        });
        $.ajax({
            type: 'POST',
            url: 'api/configfile/plugin.tplink.json',
            data: JSON.stringify(config),
            processData: false,
            contentType: 'application/json',
            success: function () {
                $(button).replaceWith('Added');
                SetRestartFlag(2);
            },
            error: function () {
                $(button).prop('disabled', false).val('Add failed - retry');
            }
        });
    };
    $.ajax({ url: 'api/configfile/plugin.tplink.json', dataType: 'json', cache: false })
        .done(save)
        .fail(function (xhr) {
            if (xhr.status == 404) {
                save([]);
            } else {
                $(button).prop('disabled', false).val('Add failed - retry');
            }
        });
}
</script>
<?php
            echo "<p>Added devices get start channel 0 (commands only); set a start channel on the Config page to drive a plug from a sequence.</p>\n";
            echo "<p>Tapo devices don't share their name until FPP logs in, so the Name column is blank for them. For a power strip, set Plug ID to the outlet number.</p>\n";
        }
    }
}
?>
        <a class='buttons genericButton' href='plugin.php?plugin=fpp-plugin-tplink&amp;page=about.php&amp;discover=1'>Run Discovery</a>
      </div>
    </fieldset>
  </div>
