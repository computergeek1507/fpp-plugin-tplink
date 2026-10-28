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
            echo "<table class='fppSelectableRowTable' style='width:100%'><thead><tr class='fppTableHeader'>";
            foreach (array('IP', 'Name', 'Model', 'Device Type', 'Outlets', 'Protocol', 'MAC', 'Use Plugin Type') as $h) {
                echo "<th>" . $h . "</th>";
            }
            echo "</tr></thead><tbody>\n";
            foreach ($found as $d) {
                echo "<tr>";
                foreach (array('ip', 'name', 'model', 'type', 'outlets', 'protocol', 'mac', 'suggest') as $k) {
                    $v = ($k == 'outlets' && !$d[$k]) ? '' : $d[$k];
                    echo "<td>" . htmlspecialchars((string)$v, ENT_QUOTES, 'UTF-8') . "</td>";
                }
                echo "</tr>\n";
            }
            echo "</tbody></table>\n";
            echo "<p>Tapo devices don't share their name until FPP logs in, so the Name column is blank for them. For a power strip, set Plug ID to the outlet number.</p>\n";
        }
    }
}
?>
        <a class='buttons genericButton' href='plugin.php?plugin=fpp-plugin-tplink&amp;page=about.php&amp;discover=1'>Run Discovery</a>
      </div>
    </fieldset>
  </div>
