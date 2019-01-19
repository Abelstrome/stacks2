<?php

/*

These functons are used to tidy the database.

*/

error_reporting(E_ALL)."<br/>";

//Global variables for database queries
$qryLogMessage; $qryTidyLog; $qryCountGames;

//for message logging
$callString;

function fnDbConnect() {
	global $qryLogMessage, $qryCountGames, $qryTidyLog;
	global $callString;
		
	require '/home/u170908969/Alison/blah.ini';
	$servername = Servername;
	$username = Username;
	$password = Password;
	$dbname = Dbname;
		
	try {
		$conn = new PDO("mysql:host=$servername;dbname=".$dbname, $username, $password);
		// set the PDO error mode to exception
		$conn->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
		//echo "Connected successfully" . "<br/>"; 
		
		//prepare database queries ready for use later
		
		//store logging info
		$query = "INSERT INTO logging SET LogMessage = :logMessage, CallString = :callString";
		$qryLogMessage = $conn->prepare($query);
		
		//tidy log by deleteing messages over 1 day old
		$query = "DELETE FROM logging WHERE TimeStamp < CURRENT_DATE - INTERVAL 35 DAY";
		$qryTidyLog = $conn->prepare($query);
		
		//count number of games
		$query = "SELECT * FROM games";
		$qryCountGames = $conn->prepare($query);
		
		fnLogMessageToDb("Connection successful");
		}
	catch(PDOException $e)
		{
		//echo "<br/>" . "Connection failed: " . $e->getMessage() . "<br/>";
		fnLogMessageToDb("Connection failed: " . $e->getMessage());
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
		echo "<br/>" . "qryLogMessage failed: " . $e->getMessage() . "<br/>";
		}
}
//End fnLogMessageToDb
function fnDbDisconnect() {
	try {
		$conn=null;
		fnLogMessageToDb("Disconnected successfully");
	}
	catch(PDOException $e)
		{
		fnLogMessageToDb("Disconnect failed: " . $e->getMessage());
		}
}
//End fnDbDisconnect
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
		fnLogMessageToDb("qryTidyDbLog failed: " . $e->getMessage());
		}
}
//End fnQryTidyDbLog
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

?>