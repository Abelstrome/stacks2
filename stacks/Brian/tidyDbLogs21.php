<html>
<body>
<?php 
	//date_default_timezone_set("Europe/London");
	require "./Functions2-1.inc";
	//set_error_handler("fnErrorHandler",E_ALL);
	$callString = "cron job - tidyDbLog.php";
	fnDbConnect();
	fnPrepQuery();

//tidy logging records
	$deletedCount = fnQryTidyDbLog();
	fnLogMessageToDb("fnQryTidyDbLog " . $deletedCount . " records deleted");

//count number of game records
	$gameCount = fnQryCountGames();
  fnLogMessageToDb("fnQryCountGames " . $gameCount . " game records in database");
/*
//write email
	$headers = "MIME-Version: 1.0\r\n";
	$headers .= "Content-Type: text/html; charset=ISO-8859-1\r\n";
	Echo "last run on ".date('l jS \of F Y h:i:s A');
	mail('admin@abelstro.esy.es','Cron Job Running','tidyDbLog ran on '.date('l jS \of F Y h:i:s A')."\r\n" .$deletedCount." rows were deleted. \r\nThere are ".$gameCount.' games in the database.',$headers);
*/
//get all players and then update thier stats
	fnQryGetPlayersUpdateStats();
  
//disconnect from database
	fnDbDisconnect(); 
?>
</body>
</html>