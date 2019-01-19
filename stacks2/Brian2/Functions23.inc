<?php
function fnDbConnect() {
	global $conn;
	global $qryAddPlayer, $qrySelectPlayer;
	global $qryUpdateRefs, $qrySelectRefs;
	global $qryAddStack, $qrySelectStacks, $qryUpdateStack, $qryDeleteStacksForManyTurns, $qryDeleteStacksForOneTurn;
	global $qryAddGame, $qrySelectGame, $qrySelectGameByID, $qryUpdateGame, $qrySelectGamesForPlayer, $qryDeleteGames;
	global $qryAddCounter, $qrySelectCounters, $qryUpdateCounter, $qrySelectAllCounters, $qryDeleteCountersForManyTurns, $qryDeleteCountersForOneTurn;
	global $qrySelectOpenGames;
	global $qryAddNmxCounter, $qrySelectAllNmxCounters, $qrySelectNmxCounters, $qryUpdateNmxCounter;
	global $qryAddNmxStack, $qrySelectNmxStacks, $qryUpdateNmxStack, $qryAddNmxGame, $qrySelectNmxGame, $qryUpdateNmxGame;
	global $qryLogMessage, $qryTidyLog;
	global $callString;

	if (strtoupper(substr($_SERVER['DOCUMENT_ROOT'],17,8)) == 'ICODE-GO') {
		require $_SERVER['DOCUMENT_ROOT'].'/stacks2/Alison2.blah.ini'; }
	else if ($_SERVER['SERVER_NAME'] == 'localhost') {
		require 'C:/xampp/htdocs/stacks/Alison2/blah.ini'; }
	else {
		require '/home/stacksx3/Alison2/blah.ini'; }

	$servername = Servername;
	$username = Username;
	$password = Password;
	$dbname = Dbname;	
	
	try {
		$conn = new PDO("mysql:host=$servername;dbname=".$dbname, $username, $password);
		// set the PDO error mode to exception
		$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
		
		//prepare database queries ready for use later
		
		//store logging info
		$query = "INSERT INTO v2_logging SET LogMessage = :logMessage, CallString = :callString";
		$qryLogMessage = $conn->prepare($query);
		
		//tidy log by deleteing messages over 1 day old
		$query = "SELECT * FROM v2_logging WHERE TimeStamp < CURRENT_DATE - INTERVAL 1 DAY";
		$qryTidyLog = $conn->prepare($query);
		
		//Select references
		$query = "SELECT * from v2_reference";
		$qrySelectRefs = $conn->prepare($query);
		
		//Update references
		$query = "UPDATE v2_reference SET NextPlayerRef = :playerRef, NextGameRef = :gameRef WHERE RefRecID = 1";
		$qryUpdateRefs = $conn->prepare($query);
		
		//Select player record
		$query = "SELECT * from v2_players WHERE PlayerName = :playerName";
		$qrySelectPlayer = $conn->prepare($query);
		
		//Add new player record
		$query = "INSERT INTO v2_players SET PlayerRef = :playerRef, PlayerName = :playerName, Password = :password, Hint = :hint";
		$qryAddPlayer = $conn->prepare($query);
		
		//Add new stack record
		$query = "INSERT INTO v2_stacks SET GameRef=:gameRef, StackNum=:stackNum, StackHeight=:stackHeight, OwnerPNum=:top, Counters=:counters, TurnCount=:turnCount";
		$qryAddStack = $conn->prepare($query);
		
		//Add new nmx-stack record
		$query = "INSERT INTO nmx_stacks SET HashKey=:hashKey, GameRef=:gameRef, StackNum=:stackNum, StackHeight=:stackHeight, Top=:top, TurnCount=:turnCount";
		$qryAddNmxStack = $conn->prepare($query);
		
		//Select stack records
		$query = "SELECT * FROM v2_stacks WHERE GameRef = :gameRef AND TurnCount = :turnCount ORDER BY StackNum" ; 
		$qrySelectStacks = $conn->prepare($query);
		
		//Select nmx-stack records
		$query = "SELECT * FROM nmx_stacks WHERE HashKey = :hashKey ORDER BY StackNum" ; 
		$qrySelectNmxStacks = $conn->prepare($query);
		
		//Update stack record
		$query = "UPDATE v2_stacks SET StackNum = :stackNum, StackHeight = :stackHeight, OwnerPNum = :top, Counters = :counters, ";
		$query = $query . "TurnCount = :turnCount WHERE StackRecID = :stackRecID";
		$qryUpdateStack = $conn->prepare($query);
		
		//Update nmx-stack record
		$query = "UPDATE nmx_stacks SET StackNum = :stackNum, StackHeight = :stackHeight, OwnerPNum = :top, TurnCount = :turnCount WHERE StackRecID = :stackRecID";
		$qryUpdateNmxStack = $conn->prepare($query);
		
		//Delete stack records for multiple turns 
		$query = "DELETE FROM v2_stacks WHERE GameRecID = :gameRef AND TurnCount > :turnCount";
		$qryDeleteStacksForManyTurns = $conn->prepare($query);
		
		//Delete stack records for one turn
		$query = "DELETE FROM v2_stacks WHERE GameRecID = :gameRef AND TurnCount = :turnCount";
		$qryDeleteStacksForOneTurn = $conn->prepare($query);
		
		//Add new counters record
		$query = "INSERT INTO counters SET GameRef = :gameRef, StackNum = :stackNum, PlayerRef = :playerRef, CounterHeight = :counterHeight, TurnCount = :turnCount";
		$qryAddCounter = $conn->prepare($query);
		
		//Add new nmx_counters record
		$query = "INSERT INTO nmx_counters SET HashKey=:hashKey, GameRef=:gameRef, StackNum=:stackNum, PlayerRef=:playerRef, CounterHeight=:counterHeight, TurnCount=:turnCount";
		$qryAddNmxCounter = $conn->prepare($query);
		
		//Update counters record
		$query = "UPDATE counters SET GameRef = :gameRef, StackNum = :stackNum, PlayerRef = :playerRef, CounterHeight = :counterHeight WHERE CounterRecID = :counterRecID" ;
		$qryUpdateCounter = $conn->prepare($query);
		
		//Update nmx_counters record
		$query = "UPDATE nmx_counters SET StackNum = :stackNum, PlayerRef = :playerRef, CounterHeight = :counterHeight, TurnCount = :turnCount WHERE CounterRecID = :counterRecID" ;
		$qryUpdateNmxCounter = $conn->prepare($query);
		
		//Select counters records
		$query = "SELECT * FROM counters WHERE GameRef = :gameRef AND StackNum = :stackNum AND TurnCount = :turnCount ORDER BY CounterHeight" ;
		$qrySelectCounters = $conn->prepare($query);
		
		//Select nmx_counters records
		$query = "SELECT * FROM nmx_counters WHERE HashKey = :hashKey AND StackNum = :stackNum ORDER BY CounterHeight" ;
		$qrySelectNmxCounters = $conn->prepare($query);
		
		//Select all counters records
		$query = "SELECT * FROM counters WHERE GameRef = :gameRef AND TurnCount = :turnCount ORDER BY StackNum, CounterHeight" ;
		$qrySelectAllCounters = $conn->prepare($query);
		
		//Select all nmx_counters records
		$query = "SELECT * FROM nmx_counters WHERE HashKey = :hashKey ORDER BY StackNum, CounterHeight" ;
		$qrySelectAllNmxCounters = $conn->prepare($query);
		
		//Delete stack records for one turn
		$query = "DELETE FROM counters WHERE GameRef = :gameRef AND TurnCount = :turnCount" ;
		$qryDeleteCountersForOneTurn = $conn->prepare($query);
		
		//Delete stack records for multiple turns
		$query = "DELETE FROM counters WHERE GameRef = :gameRef AND TurnCount > :turnCount" ;
		$qryDeleteCountersForManyTurns = $conn->prepare($query);
		
		//Add new game record
		$query = "INSERT INTO v2_games SET GameRef=:gameRef, P1RecID=:p1Ref, P2RecID=:p2Ref, LastPlayerID=:lastPlayerID, StartDate=:startDate, ";
		$query = $query . "LastUpdated=:lastUpdated, AreaSize=:areaSize, AreaWidth=:areaWidth, Winner=:winner, ";
		$query = $query . "TurnCount=:turnCount, TurnCountVer=:turnCountVer";
		$qryAddGame = $conn->prepare($query);

		//Add new nmx_games record
		$query = "INSERT INTO nmx_games SET HashKey=:hashKey, GameRef=:gameRef, P1RecID=:p1Ref, P2RecID=:p2Ref, LastPlayer=:lastPlayer, AreaSize=:areaSize, ";
		$query = $query . "Winner=:winner, TurnCount=:turnCount, TurnCountVer=:turnCountVer";
		$qryAddNmxGame = $conn->prepare($query);
		
		//Update game
		$query = "UPDATE v2_games SET P1RecID=:p1Ref, P2RecID=:p2Ref, LastPlayerID=:lastPlayerID, ";
		$query = $query . "LastUpdated=:lastUpdated, Winner=:winner, TurnCount=:turnCount WHERE GameRef=:gameRef AND TurnCountVer=:turnCountVer";
		$qryUpdateGame = $conn->prepare($query);

		//Update nmx_games
		$query = "UPDATE nmx_games SET P1Ref=:p1Ref, P2RecID=:p2Ref, LastPlayer=:lastPlayer, ";
		$query = $query . "AreaSize=:areaSize, Winner=:winner, TurnCount=:turnCount, GameScore=:gameScore WHERE HashKey=:hashKey";
		$qryUpdateNmxGame = $conn->prepare($query);
		
		//Select game record
		$query = "SELECT v2_games.GameRecID, v2_games.GameRef, v2_games.P1RecID, p1.PlayerName AS P1Name, v2_games.P2RecID, p2.PlayerName AS P2Name, ";
		$query = $query . "v2_games.LastPlayerID, LP.PlayerName as LPName, v2_games.StartDate, v2_games.AreaSize, v2_games.AreaWidth, v2_games.Winner, v2_games.TurnCount ";
		$query = $query . "FROM v2_games INNER JOIN v2_players AS p1 ON v2_games.P1RecID = p1.PlayerRef ";
		$query = $query . "INNER JOIN v2_players AS p2 ON v2_games.P2RecID = p2.PlayerRef ";
		$query = $query . "INNER JOIN v2_players AS LP ON v2_games.LastPlayerID = LP.PlayerRef ";
		$query = $query . "WHERE v2_games.GameRef = :gameRef AND v2_games.TurnCountVer = :turnCountVer";
		$qrySelectGame = $conn->prepare($query);
		
		//select game record using GameRecID
		$query = "SELECT * FROM v2_games WHERE GameRecID = :gameRecID";
		$qrySelectGameByID = $conn->prepare($query);

		//select nmx_games record 
		$query = "SELECT * FROM nmx_games WHERE HashKey = :hashKey";
		$qrySelectNmxGame = $conn->prepare($query);
		
		//Select game records for player
		$query = "SELECT * FROM v2_games WHERE (P1RecID = :playerRef OR P2RecID = :playerRef) AND TurnCountVer = :turnCountVer";
		$qrySelectGamesForPlayer = $conn->prepare($query);
		
		//Delete game records
		$query = "DELETE FROM v2_games WHERE gameRef = :gameRef AND TurnCount > :turnCount";
		$qryDeleteGames = $conn->prepare($query);
		
		//Select open games and join with players table to get player name
		$query = "SELECT v2_games.GameRef, v2_games.P1RecID, players.PlayerName FROM v2_games, players WHERE v2_games.P1RecID = players.PlayerRef AND v2_games.P2RecID = 0";
		$qrySelectOpenGames = $conn->prepare($query);
	
		fnLogMessageToDb("Connection successful");
		fnQrySelectRefs();
		}
	catch(PDOException $e)
		{
		$errMsg = "Connection failed: " . $e->getMessage();
		fnCommentLog($errMsg . '\n');
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		}	
}
//End fnDbConnect
function fnLogMessageToDb ($logMessage) {
	global $qryLogMessage;
	global $callString;
	
	//execute prepared query
	try {
		//execute query
		$qryLogMessage->execute(array(':logMessage'=>$logMessage, ':callString'=>$callString));
		}
	catch(PDOException $e)
		{
		$errMsg = "qryLogMessage failed: " . $e->getMessage();
		fnCommentLog($errMsg . "\n");
		fnCommentLog("logMessage =" . $logMessage . "\n");
		Rollbar::report_exception($e, array('error message' => $errMsg));
		}
}
//End fnLogMessageToDb
function fnQryTidyDbLog () {
	fnLogMessageToDb("Start fnQryTidyDbLog");
	global $qryTidyLog; 
	//execute prepared query
	try {
		//execute query
		$qryTidyLog->execute();
		//return number of rows that were deleted
		return $qryTidyLog->rowcount();
		}
	catch(PDOException $e)
		{
		$errMsg = "qryTidyDbLog failed: " . $e->getMessage();
		fnLogMessageToDb();
		Rollbar::report_exception($e, array('error message' => $errMsg));
		}
}
//End fnQryTidyDbLog
function fnQrySelectGamesForPlayer ($playerRef, $turnCountVer=0) {
	fnLogMessageToDb("start fnQrySelectGamesForPlayer");
	global $qrySelectGamesForPlayer;

	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectGamesForPlayer->execute(array(':playerRef'=>$playerRef,':turnCountVer'=>$turnCountVer));
		// set the resulting array to associative
		$result = $qrySelectGamesForPlayer->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectGamesForPlayer->fetchAll() as $aGameRow) { 
			$gameRows[] = $aGameRow;
			$rowCount++; }
		
		if ($rowCount>0) {
			return $gameRows; }
		else {
			fnLogMessageToDb("no games found for player ref ".$playerRef);
			return "no games found for player ref ".$playerRef; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectGamesForPlayer failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectGamesForPlayer error";
		}	
}
//End fnQrySelectGamesForPlayer
function fnQrySelectGame ($gameRef, $turnCountVer=0) {
	global $qrySelectGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectGame");}

	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectGame->execute(array(':gameRef'=>$gameRef,':turnCountVer'=>$turnCountVer));
		// set the resulting array to associative
		$result = $qrySelectGame->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectGame->fetchAll() as $gameRow) { 
			$rowCount++; }
		
		if ($rowCount==1) {
			return $gameRow; }
		elseif ($rowCount>1) {
			fnLogMessageToDb("Game ".$gameRef." has ".$rowCount." duplicate rows");
			return "duplicate rows: ".$rowCount; }
		else {
			fnLogMessageToDb('fnQrySelectGame - Game ' . $gameRef . ' with turnCountVer ' . $turnCountVer . ' not found');
			return "Game ".$gameRef." not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectGame failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "Select Game error";
		}	
	fnLogMessageToDb("exit fnQrySelectGame");
}
//End fnQrySelectGame
function fnQrySelectGameByID ($gameRecID) {
	global $qrySelectGameByID, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectGameByID");}

	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectGameByID->execute(array(':gameRecID'=>$gameRecID));
		// set the resulting array to associative
		$result = $qrySelectGameByID->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectGameByID->fetchAll() as $gameRow) { 
			$rowCount++; }
		
		if ($rowCount==1) {
			return $gameRow; }
		elseif ($rowCount>1) {
			fnLogMessageToDb("Game ".$gameRecID." has ".$rowCount." duplicate rows");
			return "duplicate rows: ".$rowCount; }
		else {
			fnLogMessageToDb("Game ".$gameRecID." not found");
			return "Game ".$gameRecID." not found"; }
		}

	catch(PDOException $e)
		{
		$errMsg = "qrySelectGameByID failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectGameByID error";
		}	
	fnLogMessageToDb("exit fnQrySelectGameByID");	
}
//End fnQrySelectGameByID
function fnQrySelectNmxGame ($hashKey) {
	global $qrySelectNmxGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectNmxGame");}

	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectNmxGame->execute(array(':hashKey'=>$hashKey));
		// set the resulting array to associative
		$result = $qrySelectNmxGame->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectNmxGame->fetchAll() as $gameRow) { 
			$rowCount++; }
		
		if ($rowCount==1) {
			return $gameRow; }
		elseif ($rowCount>1) {
			fnLogMessageToDb("Game ".$hashKey." has ".$rowCount." duplicate rows");
			return "Duplicate rows: ".$rowCount; }
		else {
			fnLogMessageToDb("Game ".$hashKey." not found");
			return "Nmx game ".$hashKey." not found"; }
		}

	catch(PDOException $e)
		{
		$errMsg = "qrySelectNmxGame failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectNmxGame error";
		}	
	fnLogMessageToDb("exit fnQrySelectNmxGame");	
}
//End fnQrySelectNmxGame
function fnQryAddGame ($gameRef, $p1Ref, $p2Ref, $lastPlayerID, $startDate, $lastUpdated, $areaSize, $areaWidth, $winner, $turnCount, $turnCountVer) {
	global $qryAddGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQryAddGame");}
	//execute prepared query
	try {
		//execute query
		$qryAddGame->execute(array(':gameRef'=>$gameRef,':p1Ref'=>$p1Ref,':p2Ref'=>$p2Ref,':lastPlayerID'=>$lastPlayerID,':startDate'=>$startDate,':lastUpdated'=>$lastUpdated,':areaSize'=>$areaSize,':areaWidth'=>$areaWidth,':winner'=>$winner,':turnCount'=>$turnCount,':turnCountVer'=>$turnCountVer));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddGame failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "AddGame error " . $e->getMessage() ;
		}
}
//End fnQryAddGame
function fnQryAddNmxGame ($hashKey, $gameRef, $p1Ref, $p2Ref, $lastPlayer, $areaSize, $winner, $turnCount, $turnCountVer) {
	global $qryAddNmxGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQryAddNmxGame");}
	//execute prepared query
	try {
		//execute query
		$qryAddNmxGame->execute(array(':hashKey'=>$hashKey,':gameRef'=>$gameRef,':p1Ref'=>$p1Ref,':p2Ref'=>$p2Ref,':lastPlayer'=>$lastPlayer,':areaSize'=>$areaSize,':winner'=>$winner,':turnCount'=>$turnCount,':turnCountVer'=>$turnCountVer));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddNmxGame failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "AddNmxGame error " . $e->getMessage();
		}
}
//End fnQryAddNmxGame
function fnQryDeleteGames ($gameRef, $turnCount) {
	fnLogMessageToDb("start fnQryDeleteGames");	
	global $qryDeleteGames;
	//execute prepared query
	try {
		//execute query
		$qryDeleteGames->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		$delCount = $qryDeleteGames->rowcount();
		if ($delCount==0){throw new Exception("zero games deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryDeleteGames failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "deleteGames error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryDeleteGames failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "deleteGames error2 " . $e->getMessage() ;
		}
}
//End fnQryDeleteGames
function fnQryUpdateGame ($gameRef, $p1Ref, $p2Ref, $lastPlayerID, $lastUpdated, $winner, $turnCount, $turnCountVer=0) {
	global $qryUpdateGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQryUpdateGame");}
	$lastUpdated = date("Y-m-d H:i:s");
	//execute prepared query
	try {
		//execute query
		$qryUpdateGame->execute(array(':gameRef'=>$gameRef,':p1Ref'=>$p1Ref,':p2Ref'=>$p2Ref,':lastPlayerID'=>$lastPlayerID,':lastUpdated'=>$lastUpdated,':winner'=>$winner,':turnCount'=>$turnCount,':turnCountVer'=>$turnCountVer));
		$updateCount = $qryUpdateGame->rowcount();
		if ($updateCount==0){throw new Exception("zero games updated");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateGame failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateGame error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateGame failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateGame error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateGame
function fnQryUpdateNmxGame ($hashKey, $p1Ref, $p2Ref, $lastPlayer, $areaSize, $winner, $turnCount, $gameScore) {
	global $qryUpdateNmxGame, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQryUpdateNmxGame");}
	//$lastUpdated = date("Y-m-d H:i:s");
	//execute prepared query
	try {
		//execute query
		$qryUpdateNmxGame->execute(array(':hashKey'=>$hashKey,':p1Ref'=>$p1Ref,':p2Ref'=>$p2Ref,':lastPlayer'=>$lastPlayer,':areaSize'=>$areaSize,':winner'=>$winner,':turnCount'=>$turnCount, ':gameScore'=>$gameScore));
		$updateCount = $qryUpdateNmxGame->rowcount();
		if ($updateCount==0){throw new Exception("zero nmx games updated");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateNmxGame failed: " . $e->getMessage() . " hashkey $hashKey";
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxGame error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateNmxGame failed2: " . $e->getMessage() . " hashkey $hashKey" ;
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxGame error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateNmxGame
function fnQryUpdateStack($stackNum, $stackHeight, $top, $counters, $stackRecID, $turnCount=0) {
	global $qryUpdateStack, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQryUpdateStack");}
	//execute prepared query
	try {
		//execute query
		$qryUpdateStack->execute(array(':stackNum'=>$stackNum,':stackHeight'=>$stackHeight,':top'=>$top,':counters'=>$counters,':stackRecID'=>$stackRecID, ':turnCount'=>$turnCount));
		$updateCount = $qryUpdateStack->rowcount();
		if ($updateCount==0){throw new Exception("zero stacks updated - ID=" . $stackRecID . " SN=" . $stackNum . " SH=" . $stackHeight . " TOP=" . $top . " TC=" . $turnCount);}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateStack failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "Update Stack error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateStack failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "Update Stack error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateStack
function fnQryUpdateNmxStack($stackNum, $stackHeight, $top, $stackRecID, $turnCount=0) {
	global $qryUpdateNmxStack, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryUpdateNmxStack");}
	//execute prepared query
	try {
		//execute query
		$qryUpdateNmxStack->execute(array(':stackNum'=>$stackNum, ':stackHeight'=>$stackHeight, ':top'=>$top, ':stackRecID' => $stackRecID, ':turnCount'=>$turnCount ));
		$updateCount = $qryUpdateNmxStack->rowcount();
		if ($updateCount==0){throw new Exception("zero nmx stacks deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateNmxStack failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxStack error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateNmxStack failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxStack error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateNmxStack
function fnQryAddStack ($gameRef, $stackNum, $stackHeight, $top, $counters, $turnCount) {
	global $qryAddStack, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryAddStack GR=".$gameRef." SN=".$stackNum." SH=".$stackHeight." OPN=".$top." Cs".$counters." TC=".$turnCount);	}
	//execute prepared query
	try {
		//execute query
		$qryAddStack->execute(array(':gameRef'=>$gameRef,':stackNum'=>$stackNum,':stackHeight'=>$stackHeight,':top'=>$top,':counters'=>$counters,':turnCount'=>$turnCount));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddStack failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "addStack error " . $e->getMessage() ;
		}
}
//End fnQryAddStack
function fnQryAddNmxStack ($hashKey, $gameRef, $stackNum, $stackHeight, $top, $turnCount) {
	global $qryAddNmxStack, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryAddNmxStack HK=" . $hashKey . " SN=" . $stackNum . " SH=" . $stackHeight . " T=" . $top);	}

	//execute prepared query
	try {
		//execute query
		$qryAddNmxStack->execute(array(':hashKey'=>$hashKey,':gameRef'=>$gameRef, ':stackNum'=>$stackNum, ':stackHeight'=>$stackHeight, ':top'=>$top, ':turnCount'=>$turnCount));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddNmxStack failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "addNmxStack error " . $e->getMessage() ;
		}
}
//End fnQryAddNmxStack
function fnQryDeleteStacksForManyTurns ($gameRef, $turnCount) {
	fnLogMessageToDb("start fnQryDeleteStacksForManyTurns");	
	global $qryDeleteStacksForManyTurns;
	//execute prepared query
	try {
		//execute query
		$qryDeleteStacksForManyTurns->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		$delCount = $qryDeleteStacksForManyTurns->rowcount();
		if ($delCount==0){throw new Exception("zero stacks  deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryDeleteStacksForManyTurns failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "deleteStacks for many turns error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryDeleteStacksForManyTurns failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "deleteStacks for many turns error2 " . $e->getMessage() ;
		}
}
//End fnQryDeleteStacksForManyTurns
function fnQryDeleteStacksForOneTurn ($gameRef, $turnCount) {
	fnLogMessageToDb("start fnQryDeleteStacksForOneTurn");	
	global $qryDeleteStacksForOneTurn;
	//execute prepared query
	try {
		//execute query
		$qryDeleteStacksForOneTurn->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		$delCount = $qryDeleteStacksForOneTurn->rowcount();
		if ($delCount==0){throw new Exception("zero stacks deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryDeleteStacksForOneTurn failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Stacks for one turn error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryDeleteStacksForOneTurn failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Stacks for one turn error2 " . $e->getMessage() ;
		}
}
//End fnQryDeleteStacksForOneTurn
function fnQrySelectStacks ($gameRef, $turnCount) {
	global $qrySelectStacks, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectStacks");}

	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectStacks->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		// set the resulting array to associative
		$result = $qrySelectStacks->setFetchMode(PDO::FETCH_ASSOC); 
		$stackRows = [];
		foreach($qrySelectStacks->fetchAll() as $aStackRow) { 
			//add an extra column that will be used to flag if the row has been changed
			array_push($aStackRow,"N");
			//make the stackNum the key for each row
			$stackRows[$aStackRow['StackNum']] = $aStackRow;
			$rowCount++; }

		if ($rowCount>=1) {
			if ($logLevel>4) {fnLogMessageToDb("multiple stacks rows: ".$rowCount);}
			return $stackRows;	}
		else {
			fnLogMessageToDb("stacks not found for gameRef $gameRef and turn count $turnCount");
			return "stacks not found for gameRef $gameRef and turn count $turnCount"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectStacks failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectStacks error";
		}
}
//End fnQrySelectStacks
function fnQrySelectNmxStacks ($hashKey) {
	global $qrySelectNmxStacks, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectNmxStacks for". $hashKey . " hashKey");}
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectNmxStacks->execute(array(':hashKey'=>$hashKey));
		// set the resulting array to associative
		$result = $qrySelectNmxStacks->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectNmxStacks->fetchAll() as $aStackRow) { 
			//add an extra column that will be used to flag if the row has been changed
			array_push($aStackRow,"N");
			$stackRows[] = $aStackRow;
			$rowCount++; }
		
		if ($rowCount>=1) {
			if ($logLevel>5) {fnLogMessageToDb("multiple nmx_stacks rows: ".$rowCount);}
			return $stackRows;	}
		else {
			$errMsg = "nmx_stacks not found for hashKey ".$hashKey;
			fnLogMessageToDb($errMsg);
			Rollbar::report_exception($e, array('error message' => $errMsg));
			return "nmx_stacks not found for hashKey >" . $hashKey . "<"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectNmxStacks failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectNmxStacks error";
		}
}
//End fnQrySelectNmxStacks
function fnQryAddCounter ($gameRef, $stackNum, $playerRef, $counterHeight, $turnCount) {
	global $qryAddCounter, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryAddCounter");	}
	//execute prepared query
	try {
		//execute query
		$qryAddCounter->execute(array(':gameRef'=>$gameRef, ':stackNum'=>$stackNum, ':playerRef'=>$playerRef, ':counterHeight'=>$counterHeight, ':turnCount'=>$turnCount));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddCounter failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "AddCounter error " . $e->getMessage() ;
		}
}
//End fnQryAddCounter
function fnQryAddNmxCounter ($hashKey, $gameRef, $stackNum, $playerRef, $counterHeight, $turnCount) {
	global $qryAddNmxCounter, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryAddNmxCounter");}	
	//execute prepared query
	try {
		//execute query
		$qryAddNmxCounter->execute(array(':hashKey'=>$hashKey, ':gameRef'=>$gameRef, ':stackNum'=>$stackNum, ':playerRef'=>$playerRef, ':counterHeight'=>$counterHeight, ':turnCount'=>$turnCount));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddNmxCounter failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "AddNmxCounter error " . $e->getMessage() ;
		}
}
//End fnQryAddNmxCounter
function fnQrySelectCounters ($gameRef, $stackNum, $turnCount) {
	global $qrySelectCounters, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectCounters");}	
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectCounters->execute(array(':gameRef'=>$gameRef, ':stackNum'=>$stackNum, ':turnCount'=>$turnCount));
		// set the resulting array to associative
		$result = $qrySelectCounters->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectCounters->fetchAll() as $aCountersRow) { 
			//add an extra column that will be used to flag if the row has been changed
			array_push($aCountersRow,"N");
			$countersRows[] = $aCountersRow;
			$rowCount++; }
		
		if ($rowCount>0) {
			return $countersRows; }
		else {
			$errMsg = "counters not found for game " . $gameRef . " and stack " . $stackNum . " and turn count " . $turnCount;
			fnLogMessageToDb($errMsg);
			return "counters not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectCounters failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "Select Counters error";
		}
}
//End fnQrySelectCounters
function fnQrySelectNmxCounters ($hashKey, $stackNum) {
	global $qrySelectNmxCounters, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectNmxCounters");	}
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectNmxCounters->execute(array(':hashKey'=>$hashKey, ':stackNum'=>$stackNum));
		// set the resulting array to associative
		$result = $qrySelectNmxCounters->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectNmxCounters->fetchAll() as $aCountersRow) { 
			//add an extra column that will be used to flag if the row has been changed
			array_push($aCountersRow,"N");
			$countersRows[] = $aCountersRow;
			$rowCount++; }
		
		if ($rowCount>0) {
			return $countersRows; }
		else {
			$errMsg = "counters not found for game " . $hashKey . " and stack " . $stackNum ;
			fnLogMessageToDb($errMsg);
			return "nmx counters not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectNmxCounters failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectNmxCounters error";
		}
}
//End fnQrySelectNmxCounters
function fnQrySelectAllCounters ($gameRef, $turnCount) {
	global $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectAllCounters");}
	global $qrySelectAllCounters;
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectAllCounters->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		// set the resulting array to associative
		$result = $qrySelectAllCounters->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectAllCounters->fetchAll() as $aCountersRow) { 
			$countersRows[] = $aCountersRow;
			$rowCount++; }
		
		if ($rowCount>0) {
			return $countersRows; }
		else {
			$errMsg = "counters not found for game " . $gameRef . " and turn count " . $turnCount;
			fnLogMessageToDb($errMsg);
			return "counters not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectAllCounters failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectAllCounters error";
		}
}
//End fnQrySelectAllCounters
function fnQrySelectAllNmxCounters ($hashKey) {
	global $qrySelectAllNmxCounters, $logLevel;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectAllNmxCounters");}
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectAllNmxCounters->execute(array(':hashKey'=>$hashKey));
		// set the resulting array to associative
		$result = $qrySelectAllNmxCounters->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectAllNmxCounters->fetchAll() as $aCountersRow) { 
			$countersRows[] = $aCountersRow;
			$rowCount++; }
		
		if ($rowCount>0) {
			return $countersRows; }
		else {
			$errMsg = "nmx_counters not found for hashKey " . $hashKey;
			fnLogMessageToDb($errMsg);
			return "nmx_counters not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectAllNmxCounters failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectAllNmxCounters error";
		}
}
//End fnQrySelectAllNmxCounters
function fnQryUpdateCounters ($gameRef, $stackNum, $playerRef, $counterHeight, $counterRecID) {
	global $qryUpdateCounter, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryUpdateCounters");	}
	//execute prepared query
	try {
		//execute query
		$qryUpdateCounter->execute(array(':gameRef'=>$gameRef, ':stackNum'=>$stackNum, ':playerRef'=>$playerRef, ':counterHeight'=>$counterHeight, ':counterRecID'=>$counterRecID));
		$updateCount = $qryUpdateCounter->rowcount();
		if ($updateCount==0){throw new Exception("zero counters updated");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateCounter failed: " . $e->getMessage() ;
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateCounters error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateCounter failed2: " . $e->getMessage() ;
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateCounters error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateCounters
function fnQryUpdateNmxCounters ($stackNum, $playerRef, $counterHeight, $counterRecID, $turnCount) {
	global $qryUpdateNmxCounter, $logLevel;
	if ($logLevel>3){fnLogMessageToDb("start fnQryUpdateNmxCounters");	}
	//execute prepared query
	try {
		//execute query
		$qryUpdateNmxCounter->execute(array(':stackNum'=>$stackNum, ':playerRef'=>$playerRef, ':counterHeight'=>$counterHeight, ':counterRecID'=>$counterRecID, ':turnCount'=>$turnCount));
		$updateCount = $qryUpdateNmxCounter->rowcount();
		if ($updateCount==0){throw new Exception("zero nmx counters updated");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateNmxCounter failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxCounters error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryUpdateNmxCounter failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateNmxCounters error2 " . $e->getMessage() ;
		}
}
//End fnQryUpdateNmxCounters
function fnQryDeleteCountersForOneTurn ($gameRef, $turnCount) {
	fnLogMessageToDb("start fnQryDeleteCountersForOneTurn");	
	global $qryDeleteCountersForOneTurn;
	//execute prepared query
	try {
		//execute query
		$qryDeleteCountersForOneTurn->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		$delCount = $qryDeleteCountersForOneTurn->rowcount();
		if ($delCount==0){throw new Exception("zero counters deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryDeleteCountersForOneTurn failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Counters for one turn error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryDeleteCountersForOneTurn failed2: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Counters for one turn error2 " . $e->getMessage() ;
		}
}
//End fnQryDeleteCountersForOneTurn
function fnQryDeleteCountersForManyTurns ($gameRef, $turnCount) {
	fnLogMessageToDb("start fnQryDeleteCountersForManyTurns");	
	global $qryDeleteCountersForManyTurns;
	//execute prepared query
	try {
		//execute query
		$qryDeleteCountersForManyTurns->execute(array(':gameRef'=>$gameRef, ':turnCount'=>$turnCount));
		$delCount = $qryDeleteCountersForManyTurns->rowcount();
		if ($delCount==0){throw new Exception("zero counters deleted");}
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryDeleteCountersForManyTurns failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Counters for many turns error " . $e->getMessage() ;
		}
	catch(Exception $e)
		{
		$errMsg = "qryDeleteCountersForManyTurns failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "delete Counters for one turn error2 " . $e->getMessage() ;
		}
}
//End fnQryDeleteCountersForManyTurns
function fnQrySelectPlayer ($playerName) {
	fnLogMessageToDb("start fnQrySelectPlayer");
	global $qrySelectPlayer;
	//execute prepared query
	$rowCount=0;
	try {
		//execute query
		$qrySelectPlayer->execute(array(':playerName'=>$playerName));
		// set the resulting array to associative
		$result = $qrySelectPlayer->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectPlayer->fetchAll() as $playerRow) { 
			$rowCount++; }
		
		if ($rowCount==1) {
			return $playerRow; }
		elseif ($rowCount>1) {
			fnLogMessageToDb("duplicate rows: ".$rowCount);
			return "duplicate rows: ".$rowCount." for player name ".$playerName;}
		else {
			fnLogMessageToDb("player name - ".$playerName." - not found");
			return "player not found"; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectPlayer failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "Select Player error";
		}
}
//End fnQrySelectPlayer
function fnQryAddPlayer($playerRef,$playerName,$password,$hint) {
	fnLogMessageToDb("start fnQryAddPlayer");	
	global $qryAddPlayer;
	//execute prepared query
	try {
		//execute query
		$qryAddPlayer->execute(array(':playerRef' => $playerRef, ':playerName' => $playerName, ':password' => $password, ':hint' => $hint));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryAddPlayer failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "AddPlayer error";
		}
	return array();
}
//End fnQryAddPlayer
function fnQrySelectRefs() {
	global $qrySelectRefs, $logLevel, $pScoreFactor, $rScoreFactor, $sScoreFactor;
	if ($logLevel>3) {fnLogMessageToDb("start fnQrySelectRefs"); }
	$rowCount = 0; 
	//execute prepared query
	try {
		//execute query
		$qrySelectRefs->execute();
		// set the resulting array to associative
		$result = $qrySelectRefs->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectRefs->fetchAll() as $refsRow) { 
			$rowCount++; }
		
		if ($rowCount==1) {
			$logLevel = $refsRow['LogLevel'];
			$pScoreFactor = $refsRow['pScoreFactor'];
			$rScoreFactor = $refsRow['rScoreFactor'];
			$sScoreFactor = $refsRow['sScoreFactor'];
			return $refsRow; }
		else {
			fnLogMessageToDb("incorrect number of rows: ".$rowCount);
			return "incorrect number of rows: " . $rowCount; }
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectRefs failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "SelectRefs error";
		}
}
//End fnQrySelectRefs
function fnQryUpdateRefs($playerRef,$gameRef) {
	fnLogMessageToDb("start fnQryUpdateRefs");	
	global $qryUpdateRefs;
	//execute prepared query
	try {
		//execute query
		$qryUpdateRefs->execute(array(':playerRef' => $playerRef, ':gameRef' => $gameRef));
		return "";
		}
	catch(PDOException $e)
		{
		$errMsg = "qryUpdateRefs failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		return "UpdateRefs error " . $e->getMessage() ;
		}
}
//End fnQryUpdateRefs
function fnReferencesTable ($method) {
	fnLogMessageToDb("start fnReferencesTable");
	//get refs from db
	$refsRow = fnQrySelectRefs();
	$playerRef = $refsRow['NextPlayerRef'];
	$gameRef = $refsRow['NextGameRef'];

	//update refs as required 
	switch ($method) {
		case "new game ref":
			$nextPlayerRef = $playerRef;
			$nextGameRef= $gameRef + 1;
			break;
		
		case "new both":
			$nextPlayerRef = $playerRef+1;
			$nextGameRef= $gameRef + 1;
			break;
		
		case "new player ref":
			$nextPlayerRef = $playerRef+1;
			$nextGameRef= $gameRef;
			break;
	
		default:
			$nextPlayerRef = 0;
			$nextGameRef= 0;
			break;
	}
	
	//update db
	if ($nextGameRef==0) {
		fnLogMessageToDb("invalid method: ".$method);
		return "invalid method: ".$method; }
	else {
		fnQryUpdateRefs ($nextPlayerRef,$nextGameRef);
		return $refsRow; }
}
//End fnReferencesTable
function fnQrySelectOpenGames() {
	fnLogMessageToDb("start fnQrySelectOpenGames");
	global $qrySelectOpenGames;
	$rowCount = 0; 
	//execute prepared query
	try {
		//execute query
		$qrySelectOpenGames->execute();
		// set the resulting array to associative
		$result = $qrySelectOpenGames->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qrySelectOpenGames->fetchAll() as $aGamesRow) { 
			$openGamesRows[] = $aGamesRow;
			$rowCount++; }

		if ($rowCount>0) {
			return $openGamesRows; }
		else {
			return "no open games found";	}
		}
	catch(PDOException $e)
		{
		$errMsg = "qrySelectOpenGames failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		}	
}
//End fnQrySelectOpenGames
function fnDbDisconnect() {
	try {
		$conn=null;
		fnLogMessageToDb("Disconnected successfully");
	}
	catch(PDOException $e)
		{
		$errMsg = "Disconnect failed: " . $e->getMessage();
		fnLogMessageToDb($errMsg);
		Rollbar::report_exception($e, array('error message' => $errMsg));
		}
}
//End fnDbDisconnect
?>