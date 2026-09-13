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
// Discovery scans the network and takes several seconds, so only run it when
// asked rather than on every visit to this page.
if (isset($_GET['discover'])) {
    $discovered = trim((string)shell_exec('/home/fpp/media/plugins/fpp-plugin-tplink/env/bin/kasa 2>&1'));
    if ($discovered == '') {
        $discovered = 'No devices found (or env/bin/kasa is not installed - re-run the plugin install).';
    }
    echo "<pre>" . htmlspecialchars($discovered, ENT_QUOTES, 'UTF-8') . "</pre>\n";
}
?>
        <a class='buttons genericButton' href='plugin.php?plugin=fpp-plugin-tplink&amp;page=about.php&amp;discover=1'>Run Discovery</a>
      </div>
    </fieldset>
  </div>
