<?php

/*

These functons are used to tidy the database and to update each player's game stats. Both actions are done in batch mode by running tidyDbLogs.php as a cron job.

Currently tidying the database is limited to pruning the logging table. Records over n days old are deleted.

The player stats (games in progress, completed, won, lost and resigned early) are stored in the players table and (will be) displayed on a tab in wordpress part of the website.

*/

error_reporting(E_ALL)."<br/>";

//Global variables for database queries
//$qryLogMessage; $qryTidyLog; 
$qryCountGames; 
$qryPlayerGamesInProgress; $qryPlayerGamesCompleted; $qryPlayerGamesWon; $qryPlayerGamesLost; $qryPlayerGamesResigned;
$qryUpdatePlayerStats; $qryGetPlayers;

//for message logging
$callString;
require	"Functions3.inc";

function fnPrepQuery() {
	
	
	//global $qryLogMessage, $qryTidyLog;
	global $conn;
	global $qryCountGames;
	global $qryPlayerGamesInProgress, $qryPlayerGamesCompleted, $qryPlayerGamesWon, $qryPlayerGamesLost, $qryPlayerGamesResigned;
	global $qryUpdatePlayerStats, $qryGetPlayers ;
	global $callString;
		
	try {
		//prepare database queries ready for use later
		
		//store logging info
		//$query = "INSERT INTO logging SET LogMessage = :logMessage, CallString = :callString";
		//$qryLogMessage = $conn->prepare($query);
				
		//count number of games
		$query = "SELECT * FROM games";
		$qryCountGames = $conn->prepare($query);

		//count games in progress for a particular player
		$query = "SELECT * FROM games WHERE (P1Ref=:playerRef OR P2Ref=:playerRef) AND Winner = 0";
		$qryPlayerGamesInProgress = $conn->prepare($query);

		//count games completed by a particular player
		$query = "SELECT * FROM games WHERE (P1Ref=:playerRef OR P2Ref=:playerRef) AND Winner <> 0";
		$qryPlayerGamesCompleted = $conn->prepare($query);
		
		//count games won by a particular player
		$query = "SELECT * FROM games WHERE (P1Ref=:playerRef OR P2Ref=:playerRef) AND Winner = :playerRef";
		$qryPlayerGamesWon = $conn->prepare($query);

		//count games lost by a particular player
		$query = "SELECT * FROM games WHERE (P1Ref=:playerRef OR P2Ref=:playerRef) AND (Winner > 0 AND Winner <> :playerRef)";
		$qryPlayerGamesLost = $conn->prepare($query);

		//count games were player resigned early
		$query = "SELECT * FROM games WHERE Winner = :playerResigned" ;
		$qryPlayerGamesResigned = $conn->prepare($query);

		//get all players and then update thier stats
		$query = "SELECT * FROM players";
		$qryGetPlayers = $conn->prepare($query);

		//Update player stats record
		$query = "UPDATE players SET InProgress = :inProgress, Completed = :completed, Won=:won, Lost = :lost, ";
		$query = $query . "ResignedEarly = :resigned, LastUpdated = NOW() WHERE PlayerRef=:playerRef";
		$qryUpdatePlayerStats = $conn->prepare($query);

		fnLogMessageToDb("Connection successful");
	}
	catch(PDOException $e)
		{
		fnLogMessageToDb("Connection failed: " . $e->getMessage());
		}	
}
//End fnPrepQuery
function fnQryCountGames () {
	fnLogMessageToDb("Start fnQryCountGames");
	global $qryCountGames;
	//execute prepared query
	try {
		//execute query
		$qryCountGames->execute();
		//return number of rows that were deleted
		return $qryCountGames->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryCountGames failed: " . $e->getMessage());
		}
}
//End fnQryCountGames
function fnQryPlayerGamesInProgress ($playerRef) {
	fnLogMessageToDb("Start fnQryPlayerGamesInProgress for player ref ".$playerRef);
	global $qryPlayerGamesInProgress ;
	//execute prepared query
	try {
		//execute query
		$qryPlayerGamesInProgress ->execute(array(':playerRef'=>$playerRef));
		//return number of rows that were selected
		return $qryPlayerGamesInProgress ->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryPlayerGamesInProgress failed: " . $e->getMessage());
		}
}
//End fnQryPlayerGamesInProgress
function fnQryPlayerGamesCompleted ($playerRef) {
	fnLogMessageToDb("Start fnQryPlayerGamesCompleted for player ref ".$playerRef);
	global $qryPlayerGamesCompleted ;
	//execute prepared query
	try {
		//execute query
		$qryPlayerGamesCompleted ->execute(array(':playerRef'=>$playerRef));
		//return number of rows that were selected
		return $qryPlayerGamesCompleted ->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryPlayerGamesCompleted failed: " . $e->getMessage());
		}
}
//End fnQryPlayerGamesCompleted
function fnQryPlayerGamesWon ($playerRef) {
	fnLogMessageToDb("Start fnQryPlayerGamesWon for player ref ".$playerRef);
	global $qryPlayerGamesWon ;
	//execute prepared query
	try {
		//execute query
		$qryPlayerGamesWon ->execute(array(':playerRef'=>$playerRef));
		//return number of rows that were selected
		return $qryPlayerGamesWon ->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryPlayerGamesWon failed: " . $e->getMessage());
		}
}
//End fnQryPlayerGamesWon
function fnQryPlayerGamesLost ($playerRef) {
	fnLogMessageToDb("Start fnQryPlayerGamesLost for player ref ".$playerRef);
	global $qryPlayerGamesLost ;
  $playerResigned = 0 - $playerRef;
	//execute prepared query
	try {
		//execute query
		$qryPlayerGamesLost ->execute(array(':playerRef'=>$playerRef, ':playerResigned'=>$playerResigned));
		//return number of rows that were selected
		return $qryPlayerGamesLost ->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryPlayerGamesLost failed: " . $e->getMessage());
		}
}
//End fnQryPlayerGamesLost
function fnQryPlayerGamesResigned ($playerRef) {
	fnLogMessageToDb("Start fnQryPlayerGamesResigned for player ref ".$playerRef);
	global $qryPlayerGamesResigned ;
  $playerResigned = 0 - $playerRef;
	//execute prepared query
	try {
		//execute query
		$qryPlayerGamesResigned ->execute(array(':playerResigned'=>$playerResigned));
		//return number of rows that were selected
		return $qryPlayerGamesResigned ->rowcount();
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryPlayerGamesResigned failed: " . $e->getMessage());
		}
}
//End fnQryPlayerGamesResigned
function fnQryUpdatePlayerStats ($playerRef, $inProgress, $completed, $won, $lost, $resigned) {
	fnLogMessageToDb("Start fnQryUpdatePlayerStats for player ref ".$playerRef);
	global $qryUpdatePlayerStats ; 
	//execute prepared query
	try {
		//execute query
		$qryUpdatePlayerStats ->execute(array(':playerRef'=>$playerRef, ':completed'=>$completed, ':inProgress'=>$inProgress, ':won'=>$won, ':lost'=>$lost, ':resigned'=>$resigned));
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryUpdatePlayerStats failed: " . $e->getMessage());
		}
}
//End fnQryUpdatePlayerStats
function fnQryGetPlayersUpdateStats () {
	fnLogMessageToDb("Start fnQryGetPlayersUpdateStats ");
	global $qryGetPlayers ; 
	//execute prepared query
	$rowCount=0;
	
	try {
		//execute query
		$qryGetPlayers ->execute();
		// set the resulting array to associative
		$result = $qryGetPlayers ->setFetchMode(PDO::FETCH_ASSOC); 
		foreach($qryGetPlayers ->fetchAll() as $aPlayerRow) { 
			//calculate stats for each player and then update the players table
			$inProgress = fnQryPlayerGamesInProgress($aPlayerRow['PlayerRef']) ;
			$completed = fnQryPlayerGamesCompleted($aPlayerRow['PlayerRef']) ;
			$won = fnQryPlayerGamesWon($aPlayerRow['PlayerRef']) ;
			$lost = fnQryPlayerGamesLost($aPlayerRow['PlayerRef']) ;
			$resigned = fnQryPlayerGamesResigned($aPlayerRow['PlayerRef']) ;
			fnQryUpdatePlayerStats($aPlayerRow['PlayerRef'], $inProgress, $completed, $won, $lost, $resigned) ; }
		}
	catch(PDOException $e)
		{
		fnLogMessageToDb("qryGetPlayers failed: " . $e->getMessage());
		}
}
//End fnQryGetPlayersUpdateStats
?>