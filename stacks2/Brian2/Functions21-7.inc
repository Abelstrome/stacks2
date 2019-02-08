<?php

/* Stacks version 1-2.
Players can now move more than one counter at a time in the playing area.
In version 1-1 a player can only ever move 1 counter at a time so long the current stack was of equal or greater height as the next. Now up to n counters can be moved together so long as the current stack is n-1 counters taller than the next. (When the current and next stacks are of equal height then only 1 counter can be moved.) The ownership of the counters that are moved doesn't change so a player can move his opponent's counters.
All other rules are the same as before.

 Stacks version 1-3.
Introduce transactions so that database updates are done consistently i.e. they all work or all are rolled back.
Read only queries (SELECTs) are ok becuase they return a value which is checked for errors.
Update/Add queries don't return any data so they will need to return a value to show if they worked or not
	fnQryAddPlayer [Done]
	fnQryUpdateRefs [No change required]
	fnQryAddStack [Used in fnProcessMove (Done) and fnSetUpGameRecs/fnProcessNewP1 (Done)]
	fnQryUpdateStack [Used in fnProcessMove (Done) and fnSetUpPlayer2/fnProcessNewP2 (Done)]
	fnQryAddGame [Used in fnProcessMove (Done) and fnSetUpGameRecs/fnProcessNewP1 (Done)]
	fnQryUpdateGame [Used in fnProcessMove (Done) and fnProcessInput/RESIGN (Done)]
	fnQryAddCounter  [Used in fnProcessMove (Done), fnSetUpGameRecs/fnProcessNewP2 (Done) and fnSetUpPlayer2/fnProcessNewP2 (Done)]
	fnQryUpdateCounter [Used in fnProcessMove (done)]
	(LogMessage)
	(TidyLog)

 Stacks version 1-4.
In this version the computer - Game Central - will use minimax or negamax (nmx for short) artificial intelligence routines to decide its moves.

************
Things to do
************

	New function called fnProcessNmxMove
		Determine if GameKey is a GameRecID or a Nmx hash key - check length of the key is less than 20 [Done]
		Calculate a new hash key [Done]
		Use fnValidatePlay if the GameKey is GameRecID otherwise use a new function that does the validation for Nmx games [Done]
		Create a new function for validating Nmx games [done]
		Create a new function to make a Nmx move - fnMakeNmxMove [done]
	New function called fnEvaluateGame to calculate a score for a given negamax game. Score from the perspective of the last player. [done]
	New sql functions required to
		select nmx_counters [Done]
		select nmx_stacks [done]
		select nmx_games [Done]
		select game record using GameRecID [Done]
		Update nmx_counters, nmx-stakcs, nms-games [done]
		Store new nmx_counters, nmx-stakcs, nms-games [done]
	Need to store logging messages and (re)write them to the database AFTER the rollback in fnProcessMove [Done]
	Need to store logging messages and (re)write them to the database AFTER the rollback in fnProcessInput [Done]
	Add an extra exception to the delete and update SQL queries to make sure that the expected number of records have been deleted or updated. See delete games for example [Done]

 Stacks version 1-5.
Combined the negamax functions into a routine to work out the best next move

 Stacks version 1-6.
New rule allowing counters moved out of reserve to be advance more than one step. The aim is to get the game started more quickly.
To do:
	fnProcessMove - for moves out the reserve allow direct to any step that is less than AreaSize/2 where the steps up to and including the Target Step are empty
		Recognise that MovePos being a decimal between 0.1 and 0.9 is a move out the reserve [done]
		Recognise that if MovePos is a decimal then the part after the decimal point is the Target Step [done]

		Check that steps up to and including Target Step are empty otherwise generate an error

	fnMakeNmxMove - as above [done]

	fnProcessInput - allow input for a Move Pos to denote a move from the reserve straight to step 1, 2, 3 ... or 9
		valid value for Move Pos are any +ve integer as before plus 0.1 to 0.9 [Done]

	fnValidatePlay
		Recognise that MovePos being a decimal between 0.1 and 0.9 is a move out the reserve [done]
		Recognise that if MovePos is a decimal then the part after the decimal point is the Target Step [done]
		Check that if MovePos is a decimal then the number is zero and the decimal part is between 1 and 9 [done]
		Check that Target Step is less than AreaSize/2 otherwise generate an error [Done]
		Check that if Target Step is greater than 1 then all steps must be empty [Done]

	fnValidateNmxPlay - as fnValidatePlay [Done]

	fnGetMoves - for moves out the reserve randomly choose from valid value for Move Pos [done]


 Stacks version 2-0 (21-0).
*/
/* Stacks version 2 has a differnt playing area to the original Stacks.
The playing are is a 2 digit number e.g. 24 which means the playing area is 2 rows or Tracks by 4 steps.
The code is stored in folder stacks2 and stacks2/Brian2
The database structure is also different in thsi version. The Counters table has been removed and the data merged into the stacks table.
*/
/* Stacks version 2-1 (21-1).
Removed the negamax functions.
Update fnGetMoves to collect all possible moves for v2 game.
Create fnFindNextStack
Create fnEncodeBoard to change a set of stacks into a string of data
Update fnProcessMove to account for new move rules and changes to stack record and removal of counters record
Update fnCheckLoser
The index for stackRows is now the stackNum e.g. 800,801,802, ...etc (instead of 0,1,2,.. etc)

 Stacks version 2-2 (21-2).
Removed obsolete code that had been commented out using block comment markers

 Stacks version 2-3 (21-3).
Added code to run on tablet using Icode-Go web IDE
Added fnPriorityMove to check if a priority move is available. i.e. player completes own boost or blocks opponents boost
Added fnRunGCP to automatically run a GC Player against the GAMECENTRAL

 Stacks version 2-4 (21-4)

Contains code to allow program to run in batch mode (**must be called from a script called batch.php**)

 Stacks version 2-5 (21-5)
Contains code to allow P1 to use a strategy to move
*/
/* Stacks version 2-6 (21-6)
Contains code to allow P1 to use q-learning to move. See fnRunGCP2
*/
/* Stacks version 2-7 (21-7)
Contains code for Monte Carlo Tree Search
Contains code (fnSimulateMove) to simulate a game without affecting the real game stored in the database
 - The start point is an encoded board so only the identities (P1 or P2) of counters at the top of a stack are known. As required the identity of counters is chosen at random.
Contains code (fnRollout) for playing a game to the end without updaing the database.
Contains code (fnSetUpGSActions) for setting up new GSA records. See MTSC 3-2 design. Desktop change. Tablet change2 v2.
Codenvy updates
*/

//Global variables for database queries
$conn;
$qryAddPlayer; $qrySelectPlayer;
$qryUpdateRefs; $qrySelectRefs;
$qryAddStack; $qrySelectStacks; $qryUpdateStack; $qryDeleteStacksForManyTurns; $qryDeleteStacksForOneTurn;
$qryAddGame; $qrySelectGame; $qrySelectGameByID; $qryUpdateGame; $qrySelectGamesForPlayer; $qryDeleteGames;
$qryAddCounter; $qrySelectCounters; $qryUpdateCounter; $qrySelectAllCounters; $qryDeleteCountersForManyTurns; $qryDeleteCountersForOneTurn;
$qrySelectOpenGames;
$qryAddNmxCounter; $qrySelectNmxCounters; $qrySelectAllNmxCounters; $qryUpdateNmxCounter; $qryAddNmxStack; $qrySelectNmxStacks; $qryUpdateNmxStack;
$qryAddNmxGame; $qrySelectNmxGame; $qryUpdateNmxGame;
$qryLogMessage; $qryTidyLog;

//Global variables
$tbP1NewPlyrName; $tbP1NewGameRef;
$tbP2NewPlyrName; $tbP2NewGameRef;

//for message logging
$callString;
require	"Functions23-1.inc";

function fnRollout($gameState, $xGameRow, $xStackRows){
	$errCode = '';
	fnLogMessageToDb('start fnSimulateGame');

	//if xGameRow is empty then create new data for xGameRow and xStackRows
	if (count($xGameRow) == 0) fnInitialiseRows($gameState, $xGameRow, $xStackRows);

	//echo 'startup'.'<br>'.'xGameRow = '; print_r($xGameRow); echo '<br>'.'xStackRows = '; print_r($xStackRows); echo '<br>';

	$minMoves = 2 ;
	$fnResp2 = ['', ''];
	$loopCount=0;

	set_time_limit(7000);
	echo 'start time ' . date("Y-m-d H:i:s") . '<br>';
	while ($fnResp2[0] == '' and $loopCount < 1300){

		//find next move for next player
		$fnResp = fnGetMoves(0, $xGameRow, $xStackRows);
		if (is_string($fnResp)) {
			$errCode = "ROL-1";
			$errMsg = $fnResp;
			fnLogMessageToDb($errCode.$errMsg);
		}
		else {
			$movesList = $fnResp;
			$moveCount = count($movesList);
			fnLogMessageToDb('number of moves for P2=' . $moveCount . '<');
			if ($moveCount > 0) {
				//Player has moves available
				//When simulating games for Monte Carlo Tree Search when a player has very few moves available the game may be counted as lost
				if ($moveCount <= $minMoves) {
					$errCode = 'ROL-2';
					$errMsg = 'Player has ' . $moveCount . ' moves available after ' . intval($xGameRow['TurnCount'] + 1) . ' turns';
					fnLogMessageToDb($errCode.$errMsg);
				}
				else {
					//choose the next move at random
					if (shuffle($movesList)) {
						$errCode = 'ROL-3';
						$errMsg = 'next move is '. $movesList[0][1] . $movesList[0][2] ;
						fnLogMessageToDb($errCode.$errMsg);
						//echo $errMsg . '<br>' ;
					}
					else{
						//error has occurred in shuffle routine
						$errCode = 'ROL-4';
						$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
						fnLogMessageToDb($errCode.$errMsg);
					}
				}
			}
		}

		$fnResp2 = fnSimulateMove($gameState, $xGameRow, $xStackRows, $movesList[0][1], $movesList[0][2], []);

		//echo 'fnSimulateGame returned ' . $fnResp2 . count($fnResp2) . '<br>';
		$loopCount++;
	}
	echo 'end time ' . date("Y-m-d H:i:s") . ' turn count ' . $xGameRow['TurnCount'] . ' result ' . $fnResp2[1];
}
//End fnRollout
function fnInitialiseRows($gameState, &$xGameRow, &$xStackRows){
	fnLogMessageToDb('start fnInitialiseRows');
	$errCode = '';
	if (count($xGameRow) == 0) {
		//populate xGameRow with default values
		//$xGameRow['GameRecID'] = -1;
		//$xGameRow['GameRef'] = -1;
		$xGameRow['Winner'] = 0;
		$xGameRow['P1RecID']  = 1;
		$xGameRow['P2RecID']  = 2;
		$xGameRow['TurnCount'] = 0;
		$xGameRow['AreaSize'] = 0;
		$xGameRow['AreaWidth'] = 0;
		//temporary game state string with an additional 'x' at the end to help with unpacking
		$tempGS = $gameState . 'x';
		if (substr_count($tempGS, 'x') % 3 > 0) {
			//the number of items should be a multiple of 3 (position, owner, height)
			$errCode = 'IRO-1';
			$errMsg = ('invalid gameState:' . $gameState . '<');
			fnLogMessageToDb($errCode.$errMsg);
		}
		$startPos = 0;
		while (strpos($tempGS, 'x', $startPos) and $errCode == '') {
			//get next set of position, owner, height details
			$endPos = strpos($tempGS, 'x', $startPos);
			$tempGSPos = substr($tempGS, $startPos, $endPos - $startPos);
			$startPos = $endPos + 1;
			$endPos = strpos($tempGS, 'x', $startPos);
			$tempGSOwner = substr($tempGS, $startPos, $endPos - $startPos);
			$startPos = $endPos + 1;
			$endPos = strpos($tempGS, 'x', $startPos);
			$tempGSHeight = intval(substr($tempGS, $startPos, $endPos - $startPos));
			$startPos = $endPos + 1;

			//extract trackNum and stepNum from position. (track is second digit, step is 3 digit onwards)
			$trackNum = intval(substr($tempGSPos, 1, 1));
			$stepNum = intval(substr($tempGSPos, 2, strlen($tempGSPos)-2));

			//update AreaWidth and AreaSize
			if ($trackNum + 1 > $xGameRow['AreaWidth']) $xGameRow['AreaWidth'] = $trackNum + 1;
			if ($stepNum - 1 > $xGameRow['AreaSize'] ) $xGameRow['AreaSize']  = $stepNum - 1;

			//populate xStackRows
			$stackNum = intval($tempGSPos);
			//$xStackRows[$stackNum]['StackRecID'] = -1;
			//$xStackRows[$stackNum]['GameRef'] = $xGameRow['GameRef'];
			$xStackRows[$stackNum]['StackNum'] = $stackNum;
			//$xStackRows[$stackNum]['TurnCount'] = 0;
			$xStackRows[$stackNum]['OwnerPNum'] = intval($tempGSOwner);
			$xStackRows[$stackNum]['StackHeight'] = $tempGSHeight;
			//if the stack is empty then Counters is blank
			if ($tempGSHeight == 0) $xStackRows[$stackNum]['Counters'] = '';
			//if stepNum is zero then the stack is in the P1 reserve
			else if ($stepNum == 0) $xStackRows[$stackNum]['Counters'] = str_repeat('1', $tempGSHeight);
			//assume stack is in the playing area and only the identify of the top counter is known
			else $xStackRows[$stackNum]['Counters'] = str_repeat('x', $tempGSHeight - 1) . $tempGSOwner;
		}
		if ($errCode == '') {
			//the true AreaSize is known so update the counters in the P2 reserve stacks
			for ($trackIx = 0; $trackIx < $xGameRow['AreaWidth']; $trackIx++) {
				$stackNum = intval('8' . $trackIx . $xGameRow['AreaSize'] + 1);
				if ($xStackRows[$stackNum]['StackHeight'] > 0) $xStackRows[$stackNum]['Counters'] = str_repeat('2', $xStackRows[$stackNum]['StackHeight']);
			}
			$xGameRow['LastPlayerID'] = 2;
		}
	}
}
//End fnInitialiseRows
function fnSimulateMove($gameState, &$xGameRow, &$xStackRows, $moveDir, $movePos, $newGameState) {
	/* This funnction is used to simlate a game.
	The start point is provided ($gameState) in the format of an encoded board and therefore only contains the onwership and height of each stack.
	If the simularted game is already in progress then xGameRow and xStackRows contain the same info as a normal game otherwise they are empty
	*/
	$errCode = ''; $errMsg = '';
	fnLogMessageToDb('start fnSimulateMove');

	//echo 'startup'.'<br>'.'xGameRow = '; print_r($xGameRow); echo '<br>'.'xStackRows = '; print_r($xStackRows); echo '<br>';

	//only continue if there are no errors
	//calculate track and stack numbers for the move
	if ($errCode == '') {
		//calculate track and stack numbers for the move
		//step num is third and any subsequent digits of the stack num
		$currStepNum = intval(substr($movePos,2,strlen($movePos)-2));
		//track is second digit of the stack num
		$currTrackNum = intval(substr($movePos,1,1));

		//calculate stackNum of next stack
		$boostMove = False;
		if ($xGameRow['LastPlayerID'] == $xGameRow['P1RecID']) {
			//player 2's move
			if ($moveDir == 3) {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == 1) {
					//boost move
					$nextStepNum = $xGameRow['AreaSize'] + 1;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just decrement stackNum
					$nextStepNum = $currStepNum - 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
		}
		else {
			//player 1's move
			if ($moveDir == 3) {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == $xGameRow['AreaSize']) {
					//boost move
					$nextStepNum = 0;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just increment stackNum
					$nextStepNum = $currStepNum + 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
		}
		//find index of next stack record
		$nextStackRowsIndex = intval('8' . strval($nextTrackNum) . strval($nextStepNum));
		$stackRowsIndex = intval('8' . strval($currTrackNum) . strval($currStepNum));
	}

	//only continue if there are no errors
	//Action the move
	if ($errCode == '') {
		if ($boostMove) {
			//move all counters
			$countToMove = $xStackRows[$stackRowsIndex]['StackHeight'];
		}
		else {
			//move top counter only
			$countToMove = 1;
		}
		//echo ' SRI=' . $stackRowsIndex . '< NSRI=' . $nextStackRowsIndex . '<';
		//move top counter(s) from current stack to next stack
		//echo '<br>'.'curr counters='.$xStackRows[$stackRowsIndex]['Counters'].'< len='.strlen($xStackRows[$stackRowsIndex]['Counters']);
		//save top counter(s)
		$topCountersString = substr($xStackRows[$stackRowsIndex]['Counters'], strlen($xStackRows[$stackRowsIndex]['Counters']) - $countToMove, $countToMove);
		//echo ' top counter(s)='.$topCountersString.'<';
		//remove top counter from current stack
		$xStackRows[$stackRowsIndex]['Counters'] = substr($xStackRows[$stackRowsIndex]['Counters'], 0, strlen($xStackRows[$stackRowsIndex]['Counters']) - $countToMove);
		//echo ' stack reduced to >' . $xStackRows[$stackRowsIndex]['Counters'] . '<';
		//reduce height of current stack
		//echo '<br>'.'heightt='.$xStackRows[$stackRowsIndex]['StackHeight'];
		$xStackRows[$stackRowsIndex]['StackHeight'] -= $countToMove;
		//echo ' then reduces to '.$xStackRows[$stackRowsIndex]['StackHeight'];
		//update owner of current stack
		//echo '<br>'.'stack owner='.$xStackRows[$stackRowsIndex]['OwnerPNum'];
		if ($boostMove or $xStackRows[$stackRowsIndex]['StackHeight'] == 0) {
			$xStackRows[$stackRowsIndex]['OwnerPNum'] = 0; }
		else {
			$xStackRows[$stackRowsIndex]['OwnerPNum'] =
				intval(substr($xStackRows[$stackRowsIndex]['Counters'], strlen($xStackRows[$stackRowsIndex]['Counters']) - 1, 1));
			if ($xStackRows[$stackRowsIndex]['OwnerPNum'] == 'x') {
				//randomly choose owner to be P1 or P2
				$xStackRows[$stackRowsIndex]['OwnerPNum'] = random_int(1,2);
				$xStackRows[$stackRowsIndex]['Counters'] =
					substr($xStackRows[$stackRowsIndex]['Counters'], 0, strlen($xStackRows[$stackRowsIndex]['Counters']) - 1) . $xStackRows[$stackRowsIndex]['OwnerPNum'];

			}
		}
		//echo ' then changed to '.$xStackRows[$stackRowsIndex]['OwnerPNum'];

		//increase height of next stack
		//echo '<br>'.'next stack height='.$xStackRows[$nextStackRowsIndex]['StackHeight'];
		$xStackRows[$nextStackRowsIndex]['StackHeight'] += $countToMove;
		//echo ' then changed to '.$xStackRows[$nextStackRowsIndex]['StackHeight'];
		//update owner of next stack
		//echo '<br>'.'next stack owner='.$xStackRows[$nextStackRowsIndex]['OwnerPNum'];
		$xStackRows[$nextStackRowsIndex]['OwnerPNum'] = intval(substr($topCountersString,strlen($topCountersString) - 1,1));
		//echo ' then changed to '.$xStackRows[$nextStackRowsIndex]['OwnerPNum'];
		//add top counter(s) to next stack
		//echo '<br>'.'next stack counters=',$xStackRows[$nextStackRowsIndex]['Counters'].'<';
		if ($boostMove) {
			//make sure all counters belong to the owner
			$xStackRows[$nextStackRowsIndex]['Counters'] =
				str_repeat(strval($xStackRows[$nextStackRowsIndex]['OwnerPNum']), $xStackRows[$nextStackRowsIndex]['StackHeight']);
		}
		else {
			$xStackRows[$nextStackRowsIndex]['Counters'] .= $topCountersString;
			//echo '<br>'.' then changed to ',$xStackRows[$nextStackRowsIndex]['Counters'].'<';
		}
		$xGameRow['TurnCount']++;
	}

	//only continue if there are no errors
	//check to see if the next player has any moves available
	$minMoves = 1;
	if ($errCode == '') {
		if ($xGameRow['LastPlayerID'] == $xGameRow['P1RecID']) {
			//P2 has moved so check if P1 has lost
			if (fnCheckLoser(1, $xStackRows, $xGameRow)) {
				$xGameRow['Winner'] = $xGameRow['P2RecID'];
				$xGameRow['LastPlayerID'] = $xGameRow['P2RecID'];
				$errMsg = 'P2 has won the game.';
				$errCode = 'SMO-2';
				fnLogMessageToDb($errCode.$errMsg);
			}
			else {
				//P2 has moved but not won so check if P1 has any moves
				//update game record so it looks like it is P1's turn
				$xGameRow['LastPlayerID'] = $xGameRow['P2RecID'];
				//get a list of possible moves
				$fnResp = fnGetMoves(0, $xGameRow, $xStackRows);
				if (is_string($fnResp)) {
					$errCode = "SMO-3";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg);
				}
				else {
					$movesList = $fnResp;
					$moveCount = count($movesList);
					fnLogMessageToDb('number of moves for P1=' . $moveCount . '<');
					//echo 'number of moves for P1='.count($movesList).'<';
					if ($moveCount > 0) {
						//P1 has moves available so leave LastPlayerID as P2
						$xGameRow['LastPlayerID'] = $xGameRow['P2RecID'];
						//When simulating games for Monte Carlo Tree Search when a player has very few moves available the game may be counted as lost
						if ($moveCount <= $minMoves) {
							$errCode = 'SMO-4a';
							$errMsg = 'P1 has ' . $moveCount . ' moves available after ' . intval($xGameRow['TurnCount'] + 1) . ' turns';
							fnLogMessageToDb($errCode.$errMsg);
						}
					}
					else {
						//P1 has no moves available so switch LastPlayerID so that it is P2's turn
						$xGameRow['LastPlayerID'] = $xGameRow['P1RecID'];
						/*
						$errMsg = "Player " . $xGameRow['P1RecID'] . " has no moves available. Miss a turn.";
						$errCode = "SMO-4";
						fnLogMessageToDb($errCode.$errMsg);
						*/
					}
				}
			}
		}
		else {
			//P1 has moved so check if P2 has lost";
			if (fnCheckLoser(2, $xStackRows, $xGameRow)) {
				$xGameRow['Winner'] = $xGameRow['P1RecID'];
				$xGameRow['LastPlayerID'] = $xGameRow['P1RecID'];
				$errMsg = 'P1 has won the game.';
				$errCode = "SMO-5";
				fnLogMessageToDb($errCode.$errMsg);
			}
			else {
				//P1 has moved but not won so check if P2 has any moves
				//update game record so it looks like it is P2's turn
				$xGameRow['LastPlayerID'] = $xGameRow['P1RecID'];
				//get a list of possible
				$fnResp = fnGetMoves(0, $xGameRow, $xStackRows);
				if (is_string($fnResp)) {
					$errCode = "SMO-7";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					$movesList = $fnResp;
					$moveCount = count($movesList);
					fnLogMessageToDb('number of moves for P2=' . $moveCount . '<');
					//echo 'number of moves for P2='.count($movesList).'<';
					if ($moveCount > 0) {
						//P2 has moves available so leave LastPlayerID as P1
						$xGameRow['LastPlayerID'] = $xGameRow['P1RecID'];
						//When simulating games for Monte Carlo Tree Search when a player has very few moves available the game may be counted as lost
						if ($moveCount <= $minMoves) {
							$errCode = 'SMO-6a';
							$errMsg = 'P2 has ' . $moveCount . ' moves available after ' . intval($xGameRow['TurnCount'] + 1) . ' turns';
							fnLogMessageToDb($errCode.$errMsg);
						}
					}
					else {
						//P2 has no moves available so switch LastPlayerID so that it is P1's turn
						$xGameRow['LastPlayerID'] = $xGameRow['P2RecID'];
						/*
						$errMsg = "Player " . $xGameRow['P2RecID'] . " has no moves available. Miss a turn.";
						$errCode = "SMO-6";
						fnLogMessageToDb($errCode.$errMsg);
						*/
					}
				}
			}
		}
	}

	//echo 'endup'.'<br>'.'xGameRow = '; print_r($xGameRow); echo '<br>'.'xStackRows = '; print_r($xStackRows); echo '<br>';

	return array($errCode,$errMsg);
}
// End of fnSimulateMove
function RunGCP3 ($playerName, $gameRef){
	global $logLevel;
	$errCode = '';
	$errMsg = '';
	fnLogMessageToDb('(' . $logLevel . ') start fnRunGCP2 for player '. $playerName . ' and game ' . $gameRef);

	/*
	Create a new game with the given player as P1 and GAMECENTRAL as P2
	Run game for a number of turns
	P1 will use monte carlo tree search
	GAMECENTRAL will make random moves
	*/

	//get player ref for given player name
	$fnResp = fnQrySelectPlayer(trim($playerName));
	if (is_array($fnResp)) {
		$p1PlayerRef = $fnResp['PlayerRef'];}
	else {
		$errCode = 'GC3-1';
		$errMsg = $fnResp; }

	//create new game with GAMECENTRAL as the P2
	if ($errCode == '') {
		if ($gameRef == 0) {
			$fnResp = fnProcessNewP1($playerName, True);
			if (array_key_exists('errCode',$fnResp) == False) {
				//Response from fnProcessNewP1 is ok - a numeric fnResp means the new game ref has been returned instead of string error message
				$gameRef = $fnResp['gameRef'];
				echo 'new game ref = ' . $gameRef;
				//echo $errMsg;
			}
			else {
				//error occurred in fnProcessNewP1
				$errCode = 'GC3-2';
				$errMsg = $fnResp['errCode'] . $fnResp['errMsg'];
			}

			if ($errCode == '') {
				$fnResp = fnProcessNewP2($gameRef, "GAMECENTRAL", True);
				if (strlen($fnResp) > 0) {
					//errors occurred
					$errCode = 'GC3-3';
					$errMsg = $fnResp;
				}
			}
		}
	}

	//get game and stacks records
	if ($errCode == '') {
		//progress update
		fnLogMessageToDb('new game ref ' . $gameRef . ' P1 is ' . $playerName . ' P2 is GAMECENTRAL');
		//get game and stacks records
		$gameRow = fnQrySelectGame($gameRef);
		if (is_string($gameRow)) {
			$errMsg = $gameRow;
			$errCode = 'GC3-4';
			fnLogMessageToDb($errCode.$errMsg);
		}
		else {
			//get stacks
			$stackRows = fnQrySelectStacks($gameRef);
			if (is_string($stackRows)) {
				$errMsg = $stackRows ;
				$errCode = 'GC3-5';
				fnLogMessageToDb($errCode.$errMsg);
			}
		}
	}
	echo 'game ref: ' . $gameRow['GameRef'] . '<BR>';

	while ($errCode == '' and $turnCount < 10000) {
		$turnCount++;
		//stop if the game is already won
		if ($gameRow['Winner'] > 0) break;

		//is it P1 or P2's turn
		if ($gameRow['P1RecID'] == $gameRow['LastPlayerID']) {
			//P2's turn
			$player1or2 = 2; }
		else {
			//P1's turn
			$player1or2 = 1; }

		$moved = False;

		//check for a priority move for P2 (GAMECENTRAL) only
		if ($player1or2 == 2) {
			fnLogMessageToDb('calling fnPriorityMove for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
			$fnResp = fnPriorityMove($gameRow, $stackRows, $player1or2);
			if ($fnResp[0] > 0) {
				//priority move found so call fnProcessMove($gameRecID, &$gameRow, &$stackRows, $moveDir, $movePos, $silent=0)
				$fnResp2 = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $fnResp[1], $fnResp[0]);
				if ($fnResp2['errCode'] != '' and $fnResp2['errCode'] != 'PMO-4' and $fnResp2['errCode'] != 'PMO-6') {
					$errCode = 'GC3-6';
					$errMsg = $fnResp2['errCode'].$fnResp2['errMsg'] ;
					fnLogMessageToDb($errCode.$errMsg);
				}
				$moved = True;
			}
		}

		//take random move for P2 (GAMECENTRAL)
		if ($errCode == '' and $moved == False and $player1or2 == 2) {
			$randMoveDir = 0;
			$randMovePos = 0;
			fnLogMessageToDb('going to take random move for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
			//get all possible moves (fnGetMoves returns a list of moves in the form (GameRecID, MoveDir, MovePos) )
			$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
			$movesListUS = $movesList;
			if (is_string($movesList)) {
				$errCode = 'GC3-7';
				$errMsg = $movesList;
				fnLogMessageToDb($errCode.$errMsg);
			}
			else {
				fnLogMessageToDb('got move list for random move . LastPlayer=' . $gameRow['LastPlayerID'] . '< player1or2=' . $player1or2 . '< GC Player=' . $playerRef);
				//store board before move
				//$board = fnEncodeBoard($stackRows, $gameRow);
				//shuffle the table and take the first entry
				if (shuffle($movesList)) {
					//table shuffled so call fnProcessMove with first entry
					$randMovePos = $movesList[0][2];
					$randMoveDir = $movesList[0][1];
					$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $randMoveDir, $randMovePos);
					//PMO-3 = P2 has won; PMO-4 = P1 misses a turn; PMO-5 = P1 has won; PMO-6 = P2 misses a turn
					if ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
						$errCode =  'GC3-8';
						$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
						fnLogMessageToDb($errCode.$errMsg);
					}
					$moved = True;
				}
				else {
					//error has occurred in shuffle routine
					$errCode = 'GC3-9';
					$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
					fnLogMessageToDb($errCode.$errMsg);
				}
			}
		}

		//Do this for P1 only
		if ($errCode == '' and $moved == False and $player1or2 == 1) {
			$board = fnEncodeBoard($stackRows, $gameRow);
			//search to find GSAction records for current board
			$fnResp = fnQrySelectGSActionMoves($board);
			if (is_string($fnResp)) {
				//error has occurred
				$errCode = 'GC3-10';
				$errMsg = $fnResp;
				fnLogMessageToDb($errCode.$errMsg);
			}
			if ($errCode == '') {
				$gsActionsCount = count($fnResp);
                                $gsActionRows = $fnResp;
				switch ($gsActionsCount) {
					case 0: 
                        /*zero records so
                        i. Save record ID as $originalGSActionRecID 
                        ii. Make record ID the current GSA rec
                        iii. Call Expansion($gsActionRows, $currentGSARecID)
                        iv. If returned record ID > 0 then make it the current GSA rec
                            1) Call Rollout to get score for a simulated game starting from NextGameState on current GSA rec
                                a) Call Rollout (NextGameState, [], [])
                            2) Call Back Propagation to update parent records with the score from Rollout
                                a) Call fnBackPropagation($currentGSARecID, $originalGSActionRecID, score)
                        v. If returned record ID = 0 go to b. and call Selection 
                        */
						$originalGSActionRecID = $gsActionRows[0][GSARecID];
						$currentGSActionRecID = $gsActionRows[0][GSARecID];
						
						$fnResp = fnExpansion($gsActionRows, $currentGSActionRecID);
						if (is_Sring($fnResp)) {
							//error has occurred
							$errCode= 'GC3-11';
							$errMsg = $fnResp;
							fnLogMessageToDb($errCode.$errMsg);
						}	
						else {
							$gsaIndex = £fnResp;
							if ($gsaIndex > 0) 
								//***should fnExpansion return the index of the current GSA in $gsActionRows instead of a RecID?
								$currentGSActionRecID = $gsActionRows[$gsRecIndex];
								$fnResp = fnRollout($gsActionRows[0][NextGameState], [], []);
								if (is_Sring($fnResp)) {
									$errMsg = 'GC3-12';
									$errCode = $fnResp;
								}
								else {
									$score = $fnResp;
									//add code here to call fnBackPropagation
								}
							else {
								//response from fnExpansion ($gsaIndex) is zero
							}
						}
						break;

					case 1: //one record so go to Expansion

						break:

					default: //more than one record so go to Selection
						$GSActionRows = $fnResp;
						break:
				}
			}

			//only continue if there are no errors
			if ($errCode == '') {
				if ($GSActionsCount == 1) {
					//save rec ID
					$GSActionOriginalRecID = $GSA
					}
					//only parent found so create new action records for each possible move
					else {
						$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
						if (is_string($movesList)) {
							$errCode = 'GC3-13';
							$errMsg = $movesList;
							fnLogMessageToDb($errCode.$errMsg);
						}
						else {
							//start database transaction
							$conn->beginTransaction();
							foreach ($movesList as $aMove){
								//create an GSAction record
								$fnResp = fnQryAddGSAction($board, 0 , "", $aMove[1], $aMove[2]);
								if (strlen($fnResp) > 0) {
									//error has occurred
									$errCode = 'GC3-14';
									$errMsg = $fnResp;
									fnLogMessageToDb($errCode.$errMsg);
									break;
								}
							}
							//end database transaction
							if (strlen($errCode) > 0) $conn->rollback();
							else $conn->commit();
						}
					}
				}
			}
		}
	}
}
// End of fnRunGCP3
function $fnSetUpGSActions($board){
	/*
	Use this function when unable to find a GSA records for a given board (game state)
	*/
}
// End of fnSetUpGSActions
function fnRunGCP2($playerName, $gameRef) {
	global $logLevel;
	$errCode = '';
	$errMsg = '';
	fnLogMessageToDb('(' . $logLevel . ') start fnRunGCP2 for player '. $playerName . ' and game ' . $gameRef);
	/*
	Create a new game with the given player as P1 and GAMECENTRAL as P2
	Run game for a number of turns
	*/

	//get player ref for given player name
	$fnResp = fnQrySelectPlayer(trim($playerName));
	if (is_array($fnResp)) {
		$playerRef = $fnResp['PlayerRef'];}
	else {
		$errCode = 'GC2-1';
		$errMsg = $fnResp; }

	//create new game with GAMECENTRAL as the P2
	if ($errCode == '') {
		if ($gameRef == 0) {
			$fnResp = fnProcessNewP1($playerName, True);
			if (array_key_exists('errCode',$fnResp) == False) {
				//Response from fnProcessNewP1 is ok - a numeric fnResp means the new game ref has been returned instead of string error message
				$gameRef = $fnResp['gameRef'];
				echo 'new game ref = ' . $gameRef;
				//echo $errMsg;
			}
			else {
				//error occurred in fnProcessNewP1
				$errCode = 'GC2-2';
				$errMsg = $fnResp['errCode'] . $fnResp['errMsg'];
			}

			if ($errCode == '') {
				$fnResp = fnProcessNewP2($gameRef, "GAMECENTRAL", True);
				if (strlen($fnResp) > 0) {
					//errors occurred
					$errCode = 'GC2-3';
					$errMsg = $fnResp;
				}
			}
		}
	}

	//get GC player data
	$qStore = [];
	if ($errCode == '') {
		$qStore = fnQrySelectGCPlayerData2($playerRef);
		if (is_string($qStore)) {
			$errCode = 'GC2-6';
			$errMsg = $qStore;
			fnLogMessageToDb($errCode.$errMsg);
		}
		else {
			echo 'got gc recs=' . count($qStore) . '<br>';
		}
	}

	//get game and stacks records
	if ($errCode == '') {
		//progress update
		fnLogMessageToDb('new game ref ' . $gameRef . ' P1 is ' . $playerName . ' P2 is GAMECENTRAL');
		//get game and stacks records
		$gameRow = fnQrySelectGame($gameRef);
		if (is_string($gameRow)) {
			$errMsg = $gameRow;
			$errCode = 'GC2-4';
			fnLogMessageToDb($errCode.$errMsg);
		}
		else {
			//get stacks
			$stackRows = fnQrySelectStacks($gameRef);
			if (is_string($stackRows)) {
				$errMsg = $stackRows ;
				$errCode = 'GC2-5';
				fnLogMessageToDb($errCode.$errMsg);
			}
		}
	}

	echo 'game ref: ' . $gameRow['GameRef'] . '<BR>';
	$turnCount = 0;
	$logLevel=3;
	while ($errCode == '' and $turnCount < 10000) {
		$turnCount++;
		//stop if the game is already won
		if ($gameRow['Winner'] > 0) break;

		//is it P1 or P2's turn
		if ($gameRow['P1RecID'] == $gameRow['LastPlayerID']) {
			//P2's turn
			$player1or2 = 2; }
		else {
			//P1's turn
			$player1or2 = 1; }

		$moved = False;

		//check for a priority move
		if ($gameRow['LastPlayerID'] != $playerRef) {
			fnLogMessageToDb('calling fnPriorityMove for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
			$fnResp = fnPriorityMove($gameRow, $stackRows, $player1or2);
			if ($fnResp[0] > 0) {
				//priority move found so call fnProcessMove($gameRecID, &$gameRow, &$stackRows, $moveDir, $movePos, $silent=0)
				$fnResp2 = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $fnResp[1], $fnResp[0]);
				if ($fnResp2['errCode'] != '' and $fnResp2['errCode'] != 'PMO-4' and $fnResp2['errCode'] != 'PMO-6') {
					$errCode = 'GC2-7';
					$errMsg = $fnResp2['errCode'].$fnResp2['errMsg'] ;
					fnLogMessageToDb($errCode.$errMsg);
				}
				$moved = True;
			}
		}

		//check GC player data
		$bestQValue = -9999;
		$bestMoveDir = 0;
		$bestMovePos = 0;
		$reward = 0;
		$calcQValue = True;
		if ($errCode == '' and $moved == False and $gameRow['LastPlayerID'] != $playerRef) {
			//not moved and GCplayer's turn so check GC player data
			fnLogMessageToDb('checking GC. LP=' . $gameRow['LastPlayerID'] . '< GCP=' . $playerRef . '<');
			$board = fnEncodeBoard($stackRows, $gameRow);

			//get move with the highest qvalue
			$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
			if (is_string($movesList)) {
				$errCode = 'GC2-A';
				$errMsg = $movesList;
				fnLogMessageToDb($errCode.$errMsg);
				$calcQValue = False;
			}
			else {
				if (shuffle($movesList)) {
					$arrayCount = count($movesList);
					for ($i = 0; $i < $arrayCount; $i++) {
						$qValue = fnGetQValue($qStore, $board, $movesList[$i][1], $movesList[$i][2], 'Y');
						if ($qValue > $bestQValue) {
							$bestQValue = $qValue;
							$bestMoveDir = $movesList[$i][1];
							$bestMovePos = $movesList[$i][2];
						}
					}
					//make best move
					$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $bestMoveDir, $bestMovePos);
					$moved = True;
					//PMO-3 = P2 has won; PMO-4 = P1 misses a turn; PMO-5 = P1 has won; PMO-6 = P2 misses a turn
					if ($fnResp['errCode'] == 'PMO-3' or  $fnResp['errCode'] == 'PMO-5') {
						//game is won to set reward to +1
						$reward = 1;
					}
					elseif ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
						$errCode = 'GC2-B';
						$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
						fnLogMessageToDb($errCode.$errMsg);
						$calcQValue = False;
					}
				}
				else {
					//error has occurred in shuffle routine
					$errCode = 'GC2-C';
					$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
					fnLogMessageToDb($errCode.$errMsg);
					$calcQValue = False;
				}
			}
			//insert here
			//calculate qValue
			if ($calcQValue) {
				//1. get the current qvalue from qStore for the move just made
				$currentQValue = $bestQValue;

				//2. get best q value for the next move but only if the next move is for the GC player otherwise best q value is zero
				//if the opponent has to miss a turn then check all of the possible moves and choose the one with the best q value in the q store
				if ($fnResp == 'PMO-4' or $fnResp == 'PMO-6') {
					//the opponent has to miss a turn so check all of the possible moves and choose the one with the best q value in the q store
					//encode board again - move has been made so board is different
					$newBoard = fnEncodeBoard($stackRows, $gameRow);
					//get move with the highest qvalue
					$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
					if (is_string($movesList)) {
						$errCode = 'GC2-D';
						$errMsg = $movesList;
						fnLogMessageToDb($errCode.$errMsg);
						$calcQValue = False;
					}
					else {
						if (shuffle($movesList)) {
							$arrayCount = count($movesList);
							$nextQValue = -9999;
							//$bestMoveDir = 0;
							//$bestMovePos = 0;
							for ($i = 0; $i < $arrayCount; $i++) {
								$qValue = fnGetQValue($qStore, $newBoard, $movesList[$i][1], $movesList[$i][2]);
								if ($qValue > $nextQValue) {
									$nextQValue = $qValue;
									//$bestMoveDir = $movesList[$i][1];
									//$bestMovePos = $movesList[$i][2];
								}
							}
						}
						else {
							//error has occurred in shuffle routine
							$errCode = 'GC2-E';
							$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
							fnLogMessageToDb($errCode.$errMsg);
							$calcQValue = False;
						}
					}
				}
				else {
					//it's the opponent's turn so next q value is zero
					$nextQValue = 0;

					//3. set the reward to -1 if the oppenent can win in the next move
					//only applicable if player has not won (reward will have already been set)
					if ($gameRow['Winner'] == 0) {
						//The opponent cannot win in the next turn if:
						// A. player has counters in the reserve stacks, or
						// B. the player owns more than one stack in the playing area, or
						// C. the player owns one stack in the playing area and the adjacent stacks are either shorter or not owned by the opponent
						$noWinningMoves = False;
						// A. check reserves
						if ($player1or2 == 1){
							if ($stackRows[800]['StackHeight'] > 0 or $stackRows[810]['StackHeight'] > 0) {
								$noWinningMoves = True;
							}
						}
						else {
							if ($stackRows[800]['StackHeight'] > 0 or $stackRows[810]['StackHeight'] > 0) {
								$noWinningMoves = True; }
						}
						// B. check number of stacks in playing area
						if ($noWinningMoves == False) {
							$stackCount = 0;
							for ($trackIx = 0; $trackIx < $gameRow['AreaWidth']; $trackIx++){
								for ($stepIx = 1; $stepIx <= $gameRow['AreaSize']; $stepIx++) {
									$stackRowsIndex = intval('8' . $trackIx . (string)$stepIx);
									if ($stackRows[$stackRowsIndex]['OwnerPNum'] == $player1or2) {
										$saveTrackIx = $trackIx;
										$saveStepIx = $stepIx;
										$stackCount++;
									}
									if ($stackCount > 1) {
										$noWinningMoves = True;
										break 2;
									}
								}
							}
						}
						// C. check around stack if there is only one stack
						if ($noWinningMoves == False) {
							$checkStacks = [];
							if ($player1or2 == 1) {
								//P1: calculate track and step index for stack ahead, stack to right and stack to left
								//stack in front
								$checkStacks[] = intval('8' . (string)$saveTrackIx . (string)$saveStepIx + 1);
								//stack to the left
								if ($saveTrackIx < $gameRow['AreaWidth'] - 1) {
									$checkStacks[] = intval('8' . (string)$saveTrackIx + 1 . (string)$saveStepIx);
								}
								//stack to the right
								if ($saveTrackIx > 0) {
									$checkStacks[] = intval('8' . (string)$saveTrackIx - 1 . (string)$saveStepIx);
								}
							}
							else {
								//P2: calculate track and step index for stack ahead, stack to right and stack to left
								//stack in front
								$checkStacks[] = intval('8' . (string)$saveTrackIx . (string)$saveStepIx - 1);
								//stack to the right
								if ($saveTrackIx < $gameRow['AreaWidth'] - 1) {
									$checkStacks[] = intval('8' . (string)$saveTrackIx + 1 . (string)$saveStepIx);
								}
								//stack to the left
								if ($saveTrackIx > 0) {
									$checkStacks[] = intval('8' . (string)$saveTrackIx - 1 . (string)$saveStepIx);
								}
							}
							//check height and ownership of stacks ahead, right and left
							//if any of the stacks are owned by the opponent and of >= height then there is a winning move
							$arrayCount = count($checkStacks);
							$saveStackRowsIndex = intval('8' . (string)$saveTrackIx . (string)$saveStepIx);
							for ($i = 0; $i < $arrayCount; $i++){
								if ($stackRows[$checkStacks[$i]]['OwnerPNum'] != $playerRef) {
									if ($stackRows[$checkStacks[$i]]['StackHeight'] >= $stackRows[$saveStackRowsIndex]['StackHeight']) {
										$noWinningMoves = True;
										break;
									}
								}
							}
						}
						//if opponent has a winning move then the reward is -1
						if ($noWinningMoves == False) $reward = -1;
					}
				}

				//calculate new q value and update q store
				if ($calcQValue) {
					$gammaFactor = 0.75;
					$alphaFactor = 0.3;
					$newQValue = ((1 - $alphaFactor) * $currentQValue) + ($alphaFactor * ($reward + ($gammaFactor * $nextQValue)));
					//update q store
					$boardMoveKey = $board . 'x' . $bestMoveDir . 'x' . $bestMovePos;
					$index = array_search($boardMoveKey, array_column($qStore, 'BoardMoveKey'));
					if (is_integer($index)) {
						if ($qStore[$index][0]== 'N') $qStore[$index][0] = 'U';
						$qStore[$index]['QValue'] = $newQValue;
						fnLogMessageToDb('QValue set to >' .  $newQValue . '< (currentQValue=' . $currentQValue . '<, reward =' . $reward . '< nextQValue=' . $nextQValue
											. '<)for boardMoveKey=' . $boardMoveKey);
					}
					else {
						//error - should have found value in q store
						$errCode = 'GC2-F';
						$errMsg = 'error has occurred unable to find ' . $boardMoveKey . ' in q store';
						fnLogMessageToDb($errCode.$errMsg);
					}
				}
			}
		}

		//take random move - will only apply to non-GC player
		$randMoveDir = 0;
		$randMovePos = 0;
		if ($errCode == '' and $moved == False) {
			fnLogMessageToDb('going to take random move for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
			//get all possible moves (fnGetMoves returns a list of moves in the form (GameRecID, MoveDir, MovePos) )
			$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
			$movesListUS = $movesList;
			if (is_string($movesList)) {
				$errCode = 'RGC-8';
				$errMsg = $movesList;
				fnLogMessageToDb($errCode.$errMsg);
				//$logMsgs[] = $errCode.$errMsg;
			}
			else {
				fnLogMessageToDb('got move list for random move . LastPlayer=' . $gameRow['LastPlayerID'] . '< player1or2=' . $player1or2 . '< GC Player=' . $playerRef);
				//store board before move
				//$board = fnEncodeBoard($stackRows, $gameRow);
				//shuffle the table and take the first entry
				if (shuffle($movesList)) {
					//table shuffled so call fnProcessMove with first entry
					$randMovePos = $movesList[0][2];
					$randMoveDir = $movesList[0][1];
					$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $randMoveDir, $randMovePos);
					//PMO-3 = P2 has won; PMO-4 = P1 misses a turn; PMO-5 = P1 has won; PMO-6 = P2 misses a turn
					if ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
						$errCode =  'RGC-9';
						$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
						fnLogMessageToDb($errCode.$errMsg);
						/*
						if ($fnResp['errCode'] = 'PMO-3' or $fnResp['errCode'] = 'PMO-5'){
							//if this player has won then the GC player has lost so the reward is -1
							$reward = -1;
						}
						*/
					}
					$moved = True;
					/*
					//only add GC player moves to table. Test is reveresed because move has been completed and LastPlayerID has been updated
					if ($errCode == '' and ($gameRow['LastPlayerID'] == $playerRef)) {
						//echo 'unshuffled moveslist: ' . print_r($movesListUS, True) . '<BR>';
						//echo '__shuffled moveslist: ' . print_r($movesList, True) . '<BR>';
						//(gcPlayerMoves[update flag, RecID, GCPlayerRef, Base, Board, MovePos, MoveDir, UsedTimes, Source])
						$gcPlayerMoves[] = array(0 => 'A', 'RecID' => 0, 'GCPlayerRef' => $playerRef, 'Base' => '', 'Board' => $board,
											'MovePos' => $movesList[0][2], 'MoveDir' => $movesList[0][1], 'UsedTimes' => 1, 'Source' => $playerRef);
						fnLogMessageToDb('count of gcPlayerMoves after adding random move =' .count($gcPlayerMoves));
					}
					*/
				}
				else {
					//error has occurred in shuffle routine
					$errCode = 'RGC-A';
					$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
					fnLogMessageToDb($errCode.$errMsg);
					//$logMsgs[] = $errCode.$errMsg;
				}
			}
		}
	}

	//store any new or updated GC data records
	$arrayCount = count($qStore);
	echo '<br>' . 'count all data records=' . $arrayCount . '<';
	$gcNewPlayerMoves = [];
	$gcUpdatedPlayerMoves = [];
	for ($i=0; $i < $arrayCount; $i++) {
		//find position of the last 'x' delimiter
		$index1 = strpos(strrev($qStore[$i]['BoardMoveKey']), 'x');
		//find position of the second last 'x' delimiter
		$index2 = strpos(strrev($qStore[$i]['BoardMoveKey']), 'x', $index1+1);
		//length of BoardMoveKey
		$keyLen = strlen($qStore[$i]['BoardMoveKey']);
		if ($qStore[$i][0] == 'U') {
			//copy updated rows in to updated moves table
			$gcUpdatedPlayerMoves[] = array('Board' => substr($qStore[$i]['BoardMoveKey'],0,$keyLen - $index2 - 1), 'GCPlayerRef' => $playerRef,
										'MoveDir' => substr($qStore[$i]['BoardMoveKey'], $keyLen - $index2, $index2 - $index1 - 1),
										'MovePos' => substr($qStore[$i]['BoardMoveKey'],$keyLen - $index1, 3),
										'UsedTimes' => $qStore[$i]['UsedTimes'], 'QValue' => $qStore[$i]['QValue']);
		}
		else if ($qStore[$i][0] == 'A') {
			//copy new rows into new moves tables
			$gcNewPlayerMoves[] = array('Board' => substr($qStore[$i]['BoardMoveKey'],0,$keyLen - $index2 - 1), 'GCPlayerRef' => $playerRef,
										'MoveDir' => substr($qStore[$i]['BoardMoveKey'], $keyLen - $index2, $index2 - $index1 - 1),
										'MovePos' => substr($qStore[$i]['BoardMoveKey'],$keyLen - $index1, 3),
										'UsedTimes' => $qStore[$i]['UsedTimes'], 'QValue' => $qStore[$i]['QValue']);
		}
	}
	echo '<br>' . 'count new=' . count($gcNewPlayerMoves) . '<' ;
	//echo '<br>' . print_r($gcNewPlayerMoves,True).'<br>';
	echo '<br>' . 'count updates=' . count($gcUpdatedPlayerMoves) . '<' ;
	//echo . '<br>' . print_r($gcUpdatedPlayerMoves,True).'<br>';
	echo '<br>' ;
	$fnResp = fnQryAddGCPlayerData2($gcNewPlayerMoves);
	if ($fnResp != '') {
		$errCode = 'RGC-B';
		$errMsg = $fnResp;
		fnLogMessageToDb($errCode.$errMsg);
	}
	else{
		$fnResp = fnQryUpdateGCPlayerData2($gcUpdatedPlayerMoves);
		if ($fnResp != '') {
			$errCode = 'RGC-C';
			$errMsg = $fnResp;
			fnLogMessageToDb($errCode.$errMsg);
		}
	}
	return $errCode.$errMsg;
}
// End of fnRunGCP2
function fnGetQValue(&$qStore, $board, $moveDir, $movePos, $updateQStore = 'N') {
	//get qvalue from qStore
	//search for match on board, moveDir and movePos
	$boardMoveKey = $board . 'x' . $moveDir . 'x' . $movePos;
	//echo ' boardMoveKey=' . $boardMoveKey . '<';
	$index = array_search($boardMoveKey, array_column($qStore, 'BoardMoveKey'));
	if (is_integer($index)) {
		$qStore[$index]['UsedTimes']++;
		if ($qStore[$index][0] == 'N' and $updateQStore = 'Y') $qStore[$index][0] = 'U';
		//echo ' updated move, usedtimes=' . $qStore[$index]['UsedTimes'] . '< ';
		return $qStore[$index]['QValue']; }
	else {
		//not found so add to the q store with a default q value of zero
		if ($updateQStore = 'Y') $qStore[] = array('BoardMoveKey' => $boardMoveKey, 'QValue' => 0, 'UsedTimes' => 1, 0 => 'A');
		return 0; }
}
// End of fnGetQValue
function fnRunGCP1($playerName, $gameRef) {
	global $logLevel;
	$errCode = '';
	fnLogMessageToDb('(' . $logLevel . ') start fnRunGCP1 for player '. $playerName . ' and game ' . $gameRef);
	/*
	Create a new game with the given player as P1 and GAMECENTRAL as P2
	Run game for a number of turns
	*/

	//get player name
	$fnResp = fnQrySelectPlayer(trim($playerName));
	if (is_array($fnResp)) {
		$playerRef = $fnResp['PlayerRef'];}
	else {
		$errCode = 'RGC-3';
		$errMsg = $fnResp; }

	/*
	//get GC player data
	$gcPlayerMoves = [];
	if ($errCode == '') {
		$gcPlayerMoves = fnQrySelectGCPlayerData($playerRef);
		echo 'got gc recs=' . count($gcPlayerMoves) . '<br>';
		//echo print_r(array_column($gcPlayerMoves,'Board')) . '<br>';
		if (is_string($gcPlayerMoves)) {
			$errCode = 'RGC-1';
			$errMsg = $gcPlayerMoves;
			fnLogMessageToDb($errCode.$errMsg);
		}
	}
	*/

	//create new game with GAMECENTRAL as the P2
	if ($errCode == '') {
		if ($gameRef == 0) {
			$fnResp = fnProcessNewP1($playerName, True);
			if (array_key_exists('errCode',$fnResp) == False) {
				//Response from fnProcessNewP1 is ok - a numeric fnResp means the new game ref has been returned instead of string error message
				$gameRef = $fnResp['gameRef'];
				$errMsg = '    new game ref = ' . $gameRef;
				//echo $errMsg;
			}
			else {
				//error occurred in fnProcessNewP1
				$errCode = 'RGC-2';
				$errMsg = $fnResp['errCode'] . $fnResp['errMsg'];
			}

			if ($errCode == '') {
				$fnResp = fnProcessNewP2($gameRef, "GAMECENTRAL", True);
				if (strlen($fnResp) > 0) {
					//errors occurred
					$errCode = 'RGC-D';
					$errMsg = $fnResp;
				}
			}
		}
	}

	//get game and stacks records
	if ($errCode == '') {
		//progress update
		fnLogMessageToDb('new game ref ' . $gameRef . ' P1 is ' . $playerName . ' P2 is GAMECENTRAL');
		//get game and stacks records
		$gameRow = fnQrySelectGame($gameRef);
		if (is_string($gameRow)) {
			$errMsg = $gameRow;
			$errCode = 'RGC-4';
			fnLogMessageToDb($errCode.$errMsg);
			//$logMsgs[] = $errCode.$errMsg;
		}
		else {
			//get stacks
			$stackRows = fnQrySelectStacks($gameRef);
			if (is_string($stackRows)) {
				$errMsg = $stackRows ;
				$errCode = 'RGC-5';
				fnLogMessageToDb($errCode.$errMsg);
				//$logMsgs[] = $errCode.$errMsg;
			}
		}
	}
	echo 'game ref: ' . $gameRow['GameRef'] . '<BR>';

	//ready to make a move
	set_time_limit(7000);
	$loopCount = 0;
	while ($errCode == '' and $loopCount < 1500) {
		$loopCount++;
		if ($gameRow['Winner'] > 0) break;
		if ($errCode == '') {
			if ($gameRow['P1RecID'] == $gameRow['LastPlayerID']) {
				//P2's turn
				$player1or2 = 2; }
			else {
				//P1's turn
				$player1or2 = 1; }
			$moved = False;
			//check for a priority move
			fnLogMessageToDb('calling fnPriorityMove for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
			$fnResp = fnPriorityMove($gameRow, $stackRows, $player1or2);
			if ($fnResp[0] > 0) {
				//priority move found so call fnProcessMove ($gameRecID, &$gameRow, &$stackRows, $moveDir, $movePos, $silent=0)
				$fnResp2 = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $fnResp[1], $fnResp[0]);
				if ($fnResp2['errCode'] != '' and $fnResp2['errCode'] != 'PMO-4' and $fnResp2['errCode'] != 'PMO-4a' and $fnResp2['errCode'] != 'PMO-6' and $fnResp2['errCode'] != 'PMO-6a') {
					$errCode = 'RGC-6';
					$errMsg = $fnResp2['errCode'].$fnResp2['errMsg'] ;
					fnLogMessageToDb($errCode.$errMsg);
					//$logMsgs[] = $errCode.$errMsg;
				}
				$moved = True;
			}
			/*
			//check GC player data
			if ($errCode == '' and $moved == False) {
				fnLogMessageToDb('checking GC. LP=' . $gameRow['LastPlayerID'] . '< GCP=' . $playerRef . '<');
				if ($gameRow['LastPlayerID'] != $playerRef) {
					fnLogMessageToDb('going to check gcPlayerMoves table');
					//not moved and GCplayer so check GC player data
					$board = fnEncodeBoard($stackRows, $gameRow);
					//search gcPlayerMoves table
					$index = array_search($board, array_column($gcPlayerMoves, 'Board'));
					fnLogMessageToDb('searched gcPlayerMoves for board=' . $board . '< index returned=' . $index . '<');
					if (is_integer($index)) {
						//update table count and mark if for update
						$gcPlayerMoves[$index]['UsedTimes'] = $gcPlayerMoves[$index]['UsedTimes'] + 1;
						$gcPlayerMoves[$index][0] = 'U';
						//match found so call fnProcessMove
						$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $gcPlayerMoves[$index]['MoveDir'], $gcPlayerMoves[$index]['MovePos']);
						if ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
							$errCode =  'RGC-7';
							$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
							fnLogMessageToDb($errCode.$errMsg);
							//$logMsgs[] = $errCode.$errMsg;
						}
						$moved = True;
						fnLogMessageToDb('finished GC player data');
					}
				}
			}
			*/
			/*
			P1 will make a move based on the following strategy:
			(1) Move to stack owned by the opponent. If there is more than one option then choose the highest stack. If stacks are the same height then move sideways
			If (1) not possible then:
			(2) Move to the highest stack possible. If stacks are the same height then move forwards. */
			if ($errCode == '' and $moved == False and random_int(1,10) < 9) {
				$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
				//get all possible moves (fnGetMoves returns a list of moves in the form (GameRecID, MoveDir, MovePos) )
				$arrayCount = count($movesList);
				if (is_string($movesList)) {
					$errCode = 'RGC-E';
					$errMsg = $movesList;
					fnLogMessageToDb($errCode.$errMsg);
				}
				else {
					if (shuffle($movesList)) {
						//echo 'moves list: ' . print_r($movesList,True) . '<br>';
						$moveScores = [];
						for ($i = 0; $i < $arrayCount; $i++) {
							//1. get trackNum and stackNum for current position
							//step num is third and any subsequent digits of the stackNum
							$currStepNum = intval(substr((string)$movesList[$i][2],2,strlen((string)$movesList[$i][2])-2));
							//track is second digit of the stackNum
							$currTrackNum = intval(substr((string)$movesList[$i][2],1,1));
							//2. construct stackRowsIndex for next stack
							if ($movesList[$i][1] == 3) {
								//decrement trackNum
								$nextTrackNum = $currTrackNum - 1;
								$nextStepNum = $currStepNum;
							}
							elseif ($movesList[$i][1] == 2) {
								//increment stackNum
								$nextStepNum = $currStepNum + 1;
								$nextTrackNum = $currTrackNum;
							}
							else {
								//increment trackNum
								$nextStepNum = $currStepNum;
								$nextTrackNum = $currTrackNum + 1;
							}
							$nextStackRowsIndex = intval('8' . strval($nextTrackNum) . strval($nextStepNum));
							//3. set scores and factors
							if ($stackRows[$nextStackRowsIndex]['OwnerPNum'] == 2) {
								//next stack belongs to P2 so set factor to 2
								$moveScores[$i] = array('Score' => $stackRows[$nextStackRowsIndex]['StackHeight'], 'Factor' => 2,
												'NSRI' => $nextStackRowsIndex, 'CSRI' => '8'.$currTrackNum.$currStepNum);
							}
							else {
								//next stack belongs to P1 so set factor and score to 1
								$moveScores[$i] = array('Score' => 1, 'Factor' => 1,
												'NSRI' => $nextStackRowsIndex, 'CSRI' => '8'.$currTrackNum.$currStepNum);
							}
						}
					}
					else {
						//error has occurred in shuffle routine
						$errCode = 'RGC-G';
						$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
						fnLogMessageToDb($errCode.$errMsg);
					}
					if ($errCode == '') {
						//find move with the best score
						//echo 'move scores: ' . print_r($moveScores,True) . '<br>';
						$bestScore=0;
						$bestScoreFactor = 1;
						$bestScoreIndex = -1;
						for ($i=0; $i < $arrayCount; $i++) {
							if ($bestScore < $moveScores[$i]['Score']) {
								//move score is better so make it the best score
								$bestScore = $moveScores[$i]['Score'];
								$bestScoreFactor = $moveScores[$i]['Factor'];
								$bestScoreIndex  = $i;
							}
							elseif ($bestScore == $moveScores[$i]['Score']) {
								//scores are equal so more tests
								if ($bestScoreFactor < $moveScores[$i]['Factor']) {
									//move score factor is higher so make it the best score
									$bestScore = $moveScores[$i]['Score'];
									$bestScoreFactor = $moveScores[$i]['Factor'];
									$bestScoreIndex  = $i;
								}
								elseif ($bestScoreFactor == $moveScores[$i]['Factor']) {
									//factors are equal so ...
									if ($moveScores[$i]['Factor'] == 1) {
										//factor indicates that next stack does belong to opponent so choose forward move
										if ($movesList[$i][1] == 2) {
											//move is forward so make it the best move
											$bestScore = $moveScores[$i]['Score'];
											$bestScoreFactor = $moveScores[$i]['Factor'];
											$bestScoreIndex  = $i;
										}
									}
									elseif ($moveScores[$i]['Factor'] == 2) {
										//factor indicates that next stack does belong to opponent so choose sideways move
										if ($movesList[$i][1] != 2) {
											//move is not forward so make it the best move
											$bestScore = $moveScores[$i]['Score'];
											$bestScoreFactor = $moveScores[$i]['Factor'];
											$bestScoreIndex  = $i;
										}
									}
								}
							}
						}
						//call fnProcessMove ($gameRecID, &$gameRow, &$stackRows, $moveDir, $movePos, $silent=0)
						$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $movesList[$bestScoreIndex][1], $movesList[$bestScoreIndex][2]);
						if ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
							$errCode =  'RGC-F';
							$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
							fnLogMessageToDb($errCode.$errMsg);
						}
					}
				}
				$moved = True;
			}
			//take random move
			if ($errCode == '' and $moved == False) {
				fnLogMessageToDb('going to take random move for P' . $player1or2 . ' and GR=' . $gameRow['GameRef'] . ' with TC=' . $gameRow['TurnCount']);
				//not moved yet so take a random move
				//get all possible moves (fnGetMoves returns a list of moves in the form (GameRecID, MoveDir, MovePos) )
				$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
				$movesListUS = $movesList;
				if (is_string($movesList)) {
					$errCode = 'RGC-8';
					$errMsg = $movesList;
					fnLogMessageToDb($errCode.$errMsg);
					//$logMsgs[] = $errCode.$errMsg;
				}
				else {
					fnLogMessageToDb('got move list for random move . LP=' . $gameRow['LastPlayerID'] . '< player1or2=' . $player1or2 . '< GC Player=' . $playerRef);
					//store board before move
					//$board = fnEncodeBoard($stackRows, $gameRow);
					//shuffle the table and take the first entry
					if (shuffle($movesList)) {
						//table shuffled so call fnProcessMove with first entry
						$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $movesList[0][1], $movesList[0][2]);
						if ($fnResp['errCode'] != '' and $fnResp['errCode'] != 'PMO-4' and $fnResp['errCode'] != 'PMO-6') {
							$errCode =  'RGC-9';
							$errMsg = $fnResp['errCode'].$fnResp['errMsg'] ;
							fnLogMessageToDb($errCode.$errMsg);
							//$logMsgs[] = $errCode.$errMsg;
						}
						$moved = True;
						/*
						//only add GC player moves to table. Test is reveresed because move has been completed and LastPlayerID has been updated
						if ($errCode == '' and ($gameRow['LastPlayerID'] == $playerRef)) {
							//echo 'unshuffled moveslist: ' . print_r($movesListUS, True) . '<BR>';
							//echo '__shuffled moveslist: ' . print_r($movesList, True) . '<BR>';
							//(gcPlayerMoves[update flag, RecID, GCPlayerRef, Base, Board, MovePos, MoveDir, UsedTimes, Source])
							$gcPlayerMoves[] = array(0 => 'A', 'RecID' => 0, 'GCPlayerRef' => $playerRef, 'Base' => '', 'Board' => $board,
												'MovePos' => $movesList[0][2], 'MoveDir' => $movesList[0][1], 'UsedTimes' => 1, 'Source' => $playerRef);
							fnLogMessageToDb('count of gcPlayerMoves after adding random move =' .count($gcPlayerMoves));
						}
						*/
					}
					else {
						//error has occurred in shuffle routine
						$errCode = 'RGC-A';
						$errMsg = 'error has occurred in shuffle routine when shuffling ' . print_r($movesList, True);
						fnLogMessageToDb($errCode.$errMsg);
						//$logMsgs[] = $errCode.$errMsg;
					}
				}
			}
		}
	}
	/*
	//store any new GC data records
	//echo 'xxx ' . print_r($gcPlayerMoves,True).'<br>';
	$arrayCount = count($gcPlayerMoves);
	echo '<br>' . 'count all data records=' . $arrayCount . '<';
	$gcNewPlayerMoves = [];
	$gcUpdatedPlayerMoves = [];
	for ($i=0; $i < $arrayCount; $i++) {
		if ($gcPlayerMoves[$i][0] == 'U') {
			//copy updated rows in to updated moves table
			$gcPlayerMoves[$i]['RecID'] = $i;
			$gcUpdatedPlayerMoves[] = $gcPlayerMoves[$i];
		}
		else if ($gcPlayerMoves[$i][0] == 'A') {
			//copy new rows into new moves tables
			$gcPlayerMoves[$i]['RecID'] = $i;
			$gcNewPlayerMoves[] = $gcPlayerMoves[$i];
		}
	}
	echo '<br>' . 'count new=' . count($gcNewPlayerMoves) . '<' ; // . print_r($gcNewPlayerMoves,True).'<br>';
	echo '<br>' . 'count updates=' . count($gcUpdatedPlayerMoves) . '<' ; //. '<br>' . print_r($gcUpdatedPlayerMoves,True).'<br>';
	echo '<br>' ;
	$fnResp = fnQryAddGCPlayerData($gcNewPlayerMoves);
	if ($fnResp != '') {
		$errCode = 'RGC-B';
		$errMsg = $fnResp;
		fnLogMessageToDb($errCode.$errMsg);
	}
	else{
		$fnResp = fnQryUpdateGCPlayerData($gcUpdatedPlayerMoves);
		if ($fnResp != '') {
			$errCode = 'RGC-C';
			$errMsg = $fnResp;
			fnLogMessageToDb($errCode.$errMsg);
		}
	} */

	return $errCode.$errMsg;
}
// End of fnRunGCP1
function fnEvaluateGame ($gameKey, $gameRow) {

	fnLogMessageToDb("start fnEvaluateGame GK=" . $gameKey) ;
	/*calcualte the score from the perspective of the next player
	pScore: Position of stacks owned relative to opponent. Score for each stack = Height/((counters)/100)*(stacknum/stackcount)
	rScore: Size of reserve relative to opponent
	sScore: Proportion of stacks owned
	*/
	global $logLevel, $pScoreFactor, $rScoreFactor, $sScoreFactor;
	$errCode="";
	//find out if gameKey is GameRecID or Nmx hash key
	if (strlen($gameKey)<20) {
		$gameRecID=$gameKey; }
	else {
		$gameRecID = 0;
		$hashKey = $gameKey; }

	//get the relevant game and stacks records
	if ($gameRecID>0) {
		//get stacks records
		if ($logLevel>2){ fnLogMessageToDb("(fnEvaluateGame) get stacks records using GameRef and TurnCount"); }
		$stackRows = fnQrySelectStacks($gameRow['GameRecID'], $gameRow['TurnCount']);
		if (is_string($stackRows)) {
			$errCode = "EVG-4";
			$errMsg = $stackRows;
			if ($logLevel>1){ fnLogMessageToDb ($errCode.$errMsg); } }
	}
	else {
		//get stacks records
		if ($logLevel>2){ fnLogMessageToDb("(fnEvaluateGame) get stacks records using hashKey"); }
		$stackRows = fnQrySelectNmxStacks($hashKey);
		if (is_string($stackRows)) {
			$errCode = "EVG-2";
			$errMsg = $stackRows;
			if ($logLevel>1){ fnLogMessageToDb ($errCode.$errMsg); } }
	}
	//check for errors
	if (strlen($errCode)==0) {
		//Calculate stack position score
		$pScore = 0; $p1StackCount = 0; $p2StackCount = 0;
		$maxI = count($stackRows) - 1;
		//loop through the stacks missing out the first (player 1 reserve) and the last (player 2 reserve)
		for ($i=1; $i<$maxI; $i++) {
			//ignore empty stacks
			if ($stackRows[$i]['OwnerPNum']>0) {
				//different calcualations for P1 and P2
				if ($stackRows[$i]['OwnerPNum'] == $gameRow['P1RecID']) {
					$p1StackCount += 1;
					//Score for each stack = Height/((counters)/100)*(stack number/(stackcount-1))
					$stackScore = $stackRows[$i]['StackHeight']/((($gameRow['AreaSize']+2)*2)/100)*($i/($gameRow['AreaSize']-1)); }
				else {
					$p2StackCount += 1;
					//Score for each stack = Height/((counters)/100)*(stack number/(stackcount-1))
					$stackScore = $stackRows[$i]['StackHeight']/((($gameRow['AreaSize']+2)*2)/100)*(($gameRow['AreaSize']-$i+1)/($gameRow['AreaSize']-1)); }
				//the score for stacks belonging to the next player are added to the overall stack score.The opponent's score are deducted
				if ($stackRows[$i]['OwnerPNum'] == $gameRow['LastPlayerID']) {
					$pScore -= $stackScore; }
				else {
					$pScore += $stackScore; }
			}
		}
		//Calculate stack score = (stacks owned by player -  stacks owned by opponent) / (stacks owned by player + stacks owned by opponent) * 100
		if ($gameRow['P2RecID'] == $gameRow['LastPlayerID']) {
			$sScore = ($p1StackCount - $p2StackCount) / ($p1StackCount + $p2StackCount) * 100; }
		else {
			$sScore = ($p2StackCount - $p1StackCount) / ($p1StackCount + $p2StackCount) * 100; }

		//Calculate reserve score = (size of player's reserve - size of opponent's reserve) / number of counters * 100
		if ($gameRow['P2RecID'] == $gameRow['LastPlayerID']) {
			$rScore = ($stackRows[0]['StackHeight'] - $stackRows[$gameRow['AreaSize']+1]['StackHeight']) / (($gameRow['AreaSize']+2)*2) * 100; }
		else {
			$rScore = ($stackRows[$gameRow['AreaSize']+1]['StackHeight'] - $stackRows[0]['StackHeight']) / (($gameRow['AreaSize']+2)*2) * 100; }

		//apply weighting factors and calculate score
		$pScore = $pScore * $pScoreFactor;
		$rScore = $rScore * $rScoreFactor;
		$sScore = $sScore * $sScoreFactor;
		$totalScore = $pScore + $rScore + $sScore;

		//update nmx game record with score
		//only update if nmx game ($gameRecID = 0) and the score has changed
		fnLogMessageToDb("GR $gameRecID , HK " . $gameRow['HashKey'] . ", totalScore $totalScore , previous score " . $gameRow['GameScore']);
		if ($gameRecID == 0 And round($totalScore,2) != $gameRow['GameScore']) {
			$fnResp = fnQryUpdateNmxGame($gameRow['HashKey'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['AreaSize'],$gameRow['Winner'],$gameRow['TurnCount'],$totalScore);
			if (strlen($fnResp) > 0) {
				$errCode = "EVG-5";
				$errMsg = $fnResp;
				fnLogMessageToDb($errCode.$errMsg);
				return array(1,$errCode.$errMsg); }
		}

		//return score
		if ($logLevel>1){ fnLogMessageToDb("fnEvaluateGame ends successfully and returns scores. pS=" . $pScore . " rS=" . $rScore . " sS=" . $sScore);}
		return array(0,round($totalScore,2));
	}
	else {
		//errors retrieving records so return error details
		if ($logLevel>2){ fnLogMessageToDb("fnEvaluateGame ends unsuccessfully and returns error");}
		return array(1,$errCode.$errMsg) ; }

}
// End fnEvaluate
function fnValidatePlayerNamePW($playerNamePW) {
	fnLogMessageToDb("Start fnValidatePlayerNamePW");
	global $logLevel;

	//separate player name and PIN
	//search for slash
	$inx = strpos($playerNamePW,"/",0);
	if ($inx == False){
		$errCode = 'VP-1';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	$playerName = substr($playerNamePW,0,$inx);
	$password = substr($playerNamePW,$inx+1,strlen($playerNamePW)-$inx);
	//search for player name
	$playerRow =fnQrySelectPlayer($playerName);
	if (is_string($playerRow)) {
		$errCode = 'VP-2';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	elseif ($password != $playerRow['Password']){
		$errCode = 'VP-3';
		$errMsg = 'Player name and\or PIN is incorrect';
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg; }
	//all good so return player ref
	return $playerRow['PlayerRef'];

}
//End fnValidatePlayerNamePW
function fnValidatePlay($gameRef, $playerRef, $player1or2, $movePos, $moveDir, $procMove = "Y") {
	/*
	validate paramters for a move.
	returns either
		a validation error message or
		nothing (if there are no errors and process move is not called) or
		the response from fnProcessMove
	*/
	fnLogMessageToDb("start fnValidatePlay");
	$errMsg = ""; $errCode = "";
	global $logLevel;

	//get game record
	if (strlen($gameRef)==0) {
		$errMsg = "Game ref not found in subValidatePlay. Fatal error.";
		$errCode = "VPY-1";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	$gameRow = fnQrySelectGame($gameRef);
	if (is_string($gameRow)) {
		$errMsg = $gameRow;
		$errCode = "VPY-2";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	if ($gameRow['Winner'] != 0){
		$errMsg = "Game " . $gameRef . " has already been won";
		$errCode = "VPY-3";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	if ($player1or2 != 1 and $player1or2 != 2) {
		$errMsg = "Incorrect player ".$playerRef;
		$errCode = "VPY-4";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	elseif (($player1or2 == 1 and $playerRef != $gameRow['P1RecID'])
		or ($player1or2 == 1 and $playerRef != $gameRow['P2RecID'])) {
		$errMsg = 'P' . $player1or2 . '-MOVE but player detail not for P' . $player1or2;
		$errCode = "VPY-6";
		fnLogMessageToDb($errCode.$errMsg); }

	//1. Check it is the right player taking a turn.
	if ($player1or2 == 1) {
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			$errMsg = "Not player 1's turn";
			$errCode = "VPY-H";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
	else {
		if ($gameRow['LastPlayerID'] == $gameRow['P2RecID']) {
			$errMsg = "Not player 2's turn";
			$errCode = "VPY-5";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}

	//if movePos must start with 8 and must contain a valid track number and step number
	$startNum = intval(substr($movePos,0,1));
	//track is second digit of the stack num
	$trackNum = intval(substr($movePos,1,1));
	//step is everything after the second digit
	$stepNum = intval(substr($movePos,2,strlen($movePos)-2));
	if (strlen($movePos) < 3 or strlen($movePos) > 4
			or $startNum != 8
			or $trackNum < 0 or $trackNum >= $gameRow['AreaWidth']
			or $stepNum < 0 or $stepNum > $gameRow['AreaSize'] + 1) {
		$errCode = 'VPY-N';
		$errMsg = 'Invalid move position - ' . $movePos;
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	/* //player must not turn in a direction that goes beyond the tracks
	if ($player1or2 == 1) {
		//player 1 cannot turn right in track 0
		if ($trackNum == 0 and $moveDir == 3) {
			$errCode = 'VPY-O';
			$errMsg = 'Invalid move direction - ' . $moveDir;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		elseif ($trackNum == $gameRow['AreaWidth']-1 and $moveDir == 1) {
			//player 1 cannot turn left in the top track
			$errCode = 'VPY-P';
			$errMsg = 'Invalid move direction - ' . $moveDir;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	}
	else {
		//player 2 cannot turn left in track 0
		if ($trackNum == 0 and $moveDir == 1) {
			$errCode = 'VPY-Q';
			$errMsg = 'Invalid move direction - ' . $moveDir;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
		elseif ($trackNum == $gameRow['AreaWidth']-1 and $moveDir == 3) {
			//player 2 cannot turn right in the top track
			$errCode = 'VPY-R';
			$errMsg = 'Invalid move direction - ' . $moveDir;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode,"errMsg"=>$errMsg); }
	} */

	//get stack records
	$stackRows = fnQrySelectStacks($gameRef,0);
	if (is_string($stackRows)) {
		$errMsg = "Stack not found in sub ValidatePlay. Fatal error.(P" . $player1or2 . ")";
		$errCode = "VPY-A";
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	//check move (movePos and moveDir) is valid
	$fnResp = fnFindNextStack($gameRow, $stackRows, $movePos, $moveDir, $player1or2);
	if ($fnResp[0] != 0) {
		$errCode = 'VPY-S';
		$errMsg = $fnResp[1].$fnResp[2];
		fnLogMessageToDb($errCode.$errMsg);
		return array("errCode"=>$errCode,"errMsg"=>$errMsg); }

	if ($procMove == "Y") {
		$fnResp = fnProcessMove($gameRow['GameRecID'], $gameRow, $stackRows, $moveDir, $movePos, True);
		if ($fnResp["errCode"] == 'PMO-4a' or $fnResp["errCode"] == 'PMO-6a') {
			$errCode = '';
			$errMsg = '';
		}
			$errCode = $fnResp["errCode"];
			$errMsg = $fnResp["errMsg"];
		}
	return array("errCode"=>$errCode,"errMsg"=>$errMsg);
}
//End fnValidatePlay
function fnProcessMove($gameRecID, &$gameRow, &$stackRows, $moveDir, $movePos, $silent=0) {
	global $conn, $logLevel;
	fnLogMessageToDb('(' . $logLevel . ') start fnProcessMove. GID='.$gameRecID.' MD='.$moveDir.' MP='.$movePos);
	$errMsg = ""; $errCode = "";
	$errCode2 = ""; $errMsg2 = "";

	/*
	Move a player's counters
	movePos is stackNum where move starts, moveDir is direction of move where 1 = left, 2 = forward, 3 = right
	returns assoicative array with error code and error message. both items are blank if move is successful
	*/

	//begin database transaction
	$conn->beginTransaction();
	$logMsgs=array("rewrite log msgs after rollback");

	if (is_array($gameRow)) {
		//game record has been passed as a parameter so don't need to go get it
		$logMsgs[] = 'gameRow passed in as a parameter';
	}
	else {
		$logMsgs[] = 'gameRow not passed in as a parameter so calling fnQrySelectGameByID with ID=' . $gameRecID . '<';
		$gameRow = fnQrySelectGameByID($gameRecID);
		if (is_string($gameRow)) {
			$errCode = "PMO-1";
			$errMsg = $gameRow;
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg; }
		else {
			if (is_array($stackRows)) {
				//stacks records have been passed as a parameter so don't need to go get them
			}
			else {
				$stackRows = fnQrySelectStacks($gameRow['GameRef'],0);
				if (is_string($stackRows)) {
					$errMsg = $stackRows;
					$errCode = "PMO-2";
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg;	}
				else{
					fnLogMessageToDb('call to fnQrySelectStacks worked. GameRef='.$gameRow['GameRef']);
					$logMsgs[] = 'call to fnQrySelectStacks worked. GameRef='.$gameRow['GameRef'];
				}
			}
		}
	}

	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//rules for moves are the same for the whole board.
		// movePos is stack where move starts, moveDir is direction of move where 1 = left, 2 = forward, 3 = right
		// need to know which player is moving in order to interpret move direction correctly

		//find index of current stack record
		$stackRowsIndex = $movePos;
	}

	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//calculate track and stack numbers
		//step num is third and any subsequent digits of the stack num
		$currStepNum = intval(substr($movePos,2,strlen($movePos)-2));
		//echo "<br>"." CS:".$currStepNum;
		//track is second digit of the stack num
		$currTrackNum = intval(substr($movePos,1,1));
		//echo ' CT:'.$currTrackNum.' MD'.$moveDir.'<br>';

		//calculate stackNum of next stack
		$boostMove = False;
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//player 2's move
			if ($moveDir == 3) {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == 1) {
					//boost move
					$nextStepNum = $gameRow['AreaSize'] + 1;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just decrement stackNum
					$nextStepNum = $currStepNum - 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
		}
		else {
			//player 1's move
			if ($moveDir == 3) {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == $gameRow['AreaSize']) {
					//boost move
					$nextStepNum = 0;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just increment stackNum
					$nextStepNum = $currStepNum + 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
		}
		//find index of next stack record
		$nextStackRowsIndex = intval('8' . strval($nextTrackNum) . strval($nextStepNum));
	}

	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		if ($boostMove) {
			//move all counters
			$countToMove = $stackRows[$stackRowsIndex]['StackHeight']; }
		else {
			//move top counter only
			$countToMove = 1; }
		//echo ' SRI=' . $stackRowsIndex . '< NSRI=' . $nextStackRowsIndex . '<';
		//move top counter(s) from current stack to next stack
		//echo '<br>'.'curr counters='.$stackRows[$stackRowsIndex]['Counters'].'< len='.strlen($stackRows[$stackRowsIndex]['Counters']);
		//save top counter(s)
		$topCountersString = substr($stackRows[$stackRowsIndex]['Counters'], strlen($stackRows[$stackRowsIndex]['Counters']) - $countToMove, $countToMove);
		//echo ' top counter(s)='.$topCountersString.'<';
		//remove top counter from current stack
		$stackRows[$stackRowsIndex]['Counters'] = substr($stackRows[$stackRowsIndex]['Counters'], 0, strlen($stackRows[$stackRowsIndex]['Counters']) - $countToMove);
		//echo ' stack reduced to >' . $stackRows[$stackRowsIndex]['Counters'] . '<';
		//reduce height of current stack
		//echo '<br>'.'heightt='.$stackRows[$stackRowsIndex]['StackHeight'];
		$stackRows[$stackRowsIndex]['StackHeight'] -= $countToMove;
		//echo ' then reduces to '.$stackRows[$stackRowsIndex]['StackHeight'];
		//update owner of current stack
		//echo '<br>'.'stack owner='.$stackRows[$stackRowsIndex]['OwnerPNum'];
		if ($boostMove or $stackRows[$stackRowsIndex]['StackHeight'] == 0) {
			$stackRows[$stackRowsIndex]['OwnerPNum'] = 0; }
		else {
			$stackRows[$stackRowsIndex]['OwnerPNum'] =
				intval(substr($stackRows[$stackRowsIndex]['Counters'], strlen($stackRows[$stackRowsIndex]['Counters']) - 1, 1)); }
		//echo ' then changed to '.$stackRows[$stackRowsIndex]['OwnerPNum'];
		//mark stack for db update
		$stackRows[$stackRowsIndex][0] = "Y";

		//increase height of next stack
		//echo '<br>'.'next stack height='.$stackRows[$nextStackRowsIndex]['StackHeight'];
		$stackRows[$nextStackRowsIndex]['StackHeight'] += $countToMove;
		//echo ' then changed to '.$stackRows[$nextStackRowsIndex]['StackHeight'];
		//update owner of next stack
		//echo '<br>'.'next stack owner='.$stackRows[$nextStackRowsIndex]['OwnerPNum'];
		$stackRows[$nextStackRowsIndex]['OwnerPNum'] = intval(substr($topCountersString,strlen($topCountersString) - 1,1));
		//echo ' then changed to '.$stackRows[$nextStackRowsIndex]['OwnerPNum'];
		//add top counter(s) to next stack
		//echo '<br>'.'next stack counters=',$stackRows[$nextStackRowsIndex]['Counters'].'<';
		if ($boostMove) {
			//make sure all counters belong to the owner
			$stackRows[$nextStackRowsIndex]['Counters'] =
				str_repeat(strval($stackRows[$nextStackRowsIndex]['OwnerPNum']), $stackRows[$nextStackRowsIndex]['StackHeight']); }
		else {
			$stackRows[$nextStackRowsIndex]['Counters'] .= $topCountersString; }
		//echo '<br>'.' then changed to ',$stackRows[$nextStackRowsIndex]['Counters'].'<';
		//mark stack for db update
		$stackRows[$nextStackRowsIndex][0] = "Y";

	}

	//only continue if there are no errors
	//check to see if the next player has any moves available
	$minMoves = 0;
	if (strlen($errCode) == 0) {
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//P2 has moved so check if P1 has lost";
			if (fnCheckLoser(1, $stackRows, $gameRow)) {
				$gameRow['Winner'] = $gameRow['P2RecID'];
				$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
				$errMsg2 = 'P2 has won the game.';
				$errCode2 = "PMO-3";
				fnLogMessageToDb($errCode2.$errMsg2);
				$logMsgs[] = $errCode2.$errMsg2; }
			else {
				//P2 has moved but not won so check if P1 has any moves";
				//update game record so it looks like it is P1's turn
				$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
				//get a list of possible moves
				$fnResp = fnGetMoves(0, $gameRow, $stackRows);
				if (is_string($fnResp)) {
					$errCode = "PMO-F";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					$movesList = $fnResp;
					$moveCount = count($movesList);
					fnLogMessageToDb('number of moves for P1=' . $moveCount . '<');
					//echo 'number of moves for P1='.count($movesList).'<';
					if ($moveCount > 0) {
						//P1 has moves available so leave LastPlayerID as P2
						$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
						//When simulating games for Monte Carlo Tree Search when a player has very few moves available the game may be counted as lost
						if ($moveCount <= $minMoves) {
							$errCode2 = 'PMO-4a';
							$errMsg2 = 'P1 has ' . $moveCount . ' moves available after ' . intval($gameRow['TurnCount'] + 1) . ' turns';
						}
					}
					else {
						//P1 has no moves available so switch LastPlayerID so that it is P2's turn
						$gameRow['LastPlayerID'] = $gameRow['P1RecID'];
						$errMsg2 = "Player " . $gameRow['P1RecID'] . " has no moves available. Miss a turn.";
						$errCode2 = "PMO-4";
						fnLogMessageToDb($errCode2.$errMsg2);
						$logMsgs[] = $errCode2.$errMsg2;
					}
				}
			}
		}
		else {
			//P1 has moved so check if P2 has lost";
			if (fnCheckLoser(2, $stackRows, $gameRow)) {
			//if (fnCheckLoser($gameRow['P2RecID'], $stackRows)) {
				$gameRow['Winner'] = $gameRow['P1RecID'];
				$gameRow['LastPlayerID'] = $gameRow['P1RecID'];
				$errMsg2 = 'P1 has won the game.';
				$errCode2 = "PMO-5";
				fnLogMessageToDb($errCode2.$errMsg2);
				$logMsgs[] = $errCode2.$errMsg2;
			}
			else {
				//P1 has moved but not won so check if P2 has any moves
				//update game record so it looks like it is P2's turn
				$gameRow['LastPlayerID'] = $gameRow['P1RecID'];
				//get a list of possible
				$fnResp = fnGetMoves(0, $gameRow, $stackRows);
				if (is_string($fnResp)) {
					$errCode = "PMO-G";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
				else {
					$movesList = $fnResp;
					$moveCount = count($movesList);
					fnLogMessageToDb('number of moves for P2=' . $moveCount . '<');
					//echo 'number of moves for P2='.count($movesList).'<';
					if ($moveCount > 0) {
						//P2 has moves available so leave LastPlayerID as P1
						$gameRow['LastPlayerID'] = $gameRow['P1RecID'];
						//When simulating games for Monte Carlo Tree Search when a player has very few moves available the game may be counted as lost
						if ($moveCount <= $minMoves) {
							$errCode2 = 'PMO-6a';
							$errMsg2 = 'P2 has ' . $moveCount . ' moves available after ' . intval($gameRow['TurnCount'] + 1) . ' turns';
						}
					}
					else {
						//P2 has no moves available so switch LastPlayerID so that it is P1's turn
						$gameRow['LastPlayerID'] = $gameRow['P2RecID'];
						$errMsg2 = "Player " . $gameRow['P2RecID'] . " has no moves available. Miss a turn.";
						$errCode2 = "PMO-6";
						fnLogMessageToDb($errCode2.$errMsg2);
						$logMsgs[] = $errCode2.$errMsg2;
					}
				}
			}
		}
	}

	//only continue if there are no errors
	//save game rec
	if (strlen($errCode) == 0) {
		if ($logLevel>2) {fnLogMessageToDb("saving game") ;}
		$logMsgs[] = "saving game";
		//update the last updated time";
		$gameRow['LastUpdated'] = date("Y-m-d H:i:s");
		//increment move count
		$gameRow['TurnCount'] += 1;
		$logMsgs[] = 'calling fnQryUpdateGame for gameRef=' . $gameRow['GameRef'] . '<';
		$fnResp = fnQryUpdateGame($gameRow['GameRef'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],date("Y-m-d H:i:s"),$gameRow['Winner'],$gameRow['TurnCount']);
		if (strlen($fnResp) > 0) {
			$errCode = "PMO-0";
			$errMsg = $fnResp;
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;
		}
		else {
			// also store a backup
			$fnResp = fnQryAddGame($gameRow['GameRef'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],$gameRow['StartDate'],$gameRow['LastUpdated'],$gameRow['AreaSize'],$gameRow['AreaWidth'],$gameRow['Winner'],$gameRow['TurnCount'],$gameRow['TurnCount']);
			if (strlen($fnResp) > 0) {
				$errCode = "PMO-A";
				$errMsg = $fnResp;
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
			}
		}
	}
	//only continue if there are no errors
	//save stack recs
	if (strlen($errCode) == 0) {
		if ($logLevel>2) {fnLogMessageToDb("saving stacks") ;}
		$logMsgs[] = "saving stacks";
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			for ($stepNum = 0; $stepNum < $gameRow['AreaSize']+2; $stepNum++) {
				//construct index
				$i = intval('8' . $trackNum . $stepNum);
				//echo 'contructed $i on line 1020=' . $i . '<';
				if ($stackRows[$i][0] == "Y") {
					$fnResp = fnQryUpdateStack($stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],$stackRows[$i]['Counters'],$stackRows[$i]['StackRecID']);
					//reset update flag
					$stackRows[$i][0] = "N";
					if (strlen($fnResp) > 0) {
						//if the stack number is the first or last and its error2 (no updates) then ignore the error
						if ((($stackRows[$i]['StackNum']==1) Or ($stackRows[$i]['StackNum']==$gameRow['AreaSize'])) And (stripos($fnResp, 'error2')>=0)) {}
						else {
							$errCode = "PMO-B";
							$errMsg = $fnResp;
							fnLogMessageToDb($errCode.$errMsg);
							$logMsgs[] = $errCode.$errMsg;
							break;
						}
					}
				}
				//also store a backup
				$fnResp = fnQryAddStack($stackRows[$i]['GameRef'],$stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],$stackRows[$i]['Counters'],$gameRow['TurnCount']);
				if (strlen($fnResp) > 0) {
					$errCode = "PMO-C";
					$errMsg = $fnResp;
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg;
					break;
				}
			}
		}
	}
	//end database transaction
	if (strlen($errCode)>0) {
		//an error has occurred rollback any database uopdates
		$conn->rollback();
		foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
		fnLogMessageToDb ("Process Move - Rollback");
		return array("errCode"=>$errCode,"errMsg"=>$errMsg);
	}
	else {
		$conn->commit();
		fnLogMessageToDb ("Process Move - Commit");
		return array("errCode"=>$errCode2,"errMsg"=>$errMsg2);
	}
}
//End fnProcessMove
function fnCopyGame($gameRef, $turnCount){
	fnLogMessageToDb("start fnCopyGame");
	/*
	make a complete copy of a game including stacks a counters records.
	use a new game ref and set turncount to zero
	*/
	global $conn, $logLevel;

	//begin database transaction
	$conn->beginTransaction();
	$logMsgs=array("rewrite log msgs after rollback");

	$gameRow = fnQrySelectGame($gameRef);
	if (is_string($gameRow)) {
		$errMsg = $gameRow;
		$errCode = "CGR-1";
		fnLogMessageToDb($errCode.$errMsg);
		$logMsgs[] = $errCode.$errMsg; }
	else {
		//get stacks
		$stackRows = fnQrySelectStacks($gameRef, $turnCount );
		if (is_string($stackRows)) {
			$errMsg = $stackRows ;
			$errCode = "CGR-2";
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;	}
		else {
			//get counters
			$countersRows = fnQrySelectAllCounters($gameRef, $turnCount );
			if (is_string($countersRows)) {
				$errMsg = $counterRows;
				$errCode = "CGR-3";
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg; }
			else {
				//already have a player ref so just get new game ref
				$refsRow = fnReferencesTable("new game ref");
				if (is_array($refsRow)) {
					$newGameRef = $refsRow['NextGameRef'];	}
				else {
					$errMsg = "error setting up new game reference";
					$errCode = "CGR-1";
					fnLogMessageToDb($errCode.$errMsg);
					$logMsgs[] = $errCode.$errMsg; }
			}
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//save counters recs with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying counters") ;}
		$logMsgs[] = "copying counters" ;
		$maxI = count($countersRows);
		for ($i=0; $i<$maxI; $i++) {
			$fnResp = fnQryAddCounter($newGameRef,$countersRows[$i]['StackNum'],$countersRows[$i]['PlayerRef'],$countersRows[$i]['CounterHeight'],0);
			if (strlen($fnResp) > 0) {
				$errCode = "CGR-2";
				$errMsg = $fnResp;
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
				break; }
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//copy game with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying game") ;}
		$logMsgs[] = "copying game" ;
		$fnResp = fnQryAddGame($newGameRef,$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],date("Y-m-d H:i:s"),date("Y-m-d H:i:s"),$gameRow['AreaSize'],$gameRow['Winner'],0,0);
		if (strlen($fnResp) > 0) {
			$errCode = "CGR-3";
			$errMsg = $fnResp;
			fnLogMessageToDb($errCode.$errMsg);
			$logMsgs[] = $errCode.$errMsg;
		}
	}
	//only continue if there are no errors
	if (strlen($errCode) == 0) {
		//copy stack recs with new game ref and zero turn count
		if ($logLevel>2) {fnLogMessageToDb("copying stacks") ;}
		$logMsgs[] = "copying stacks";
		$maxI = count($stackRows);
		for ($i=0; $i<$maxI; $i++) {
			$fnResp = fnQryAddStack($newGameRef,$stackRows[$i]['StackNum'],$stackRows[$i]['StackHeight'],$stackRows[$i]['OwnerPNum'],0);
			if (strlen($fnResp) > 0) {
				$errCode2 = "CGR-C";
				$errMsg2 = $fnResp;
				fnLogMessageToDb($errCode.$errMsg);
				$logMsgs[] = $errCode.$errMsg;
				break;
			}
		}
	}
	//end database transaction
	if (strlen($errCode)>0) {
		//an error has occurred rollback any database updates
		$conn->rollback();
		foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
		fnLogMessageToDb ("Copy game records - Rollback");
		return $errCode.$errMsg ;
	}
	else {
		$conn->commit();
		fnLogMessageToDb (" Copy game records - Commit");
		return "";
	}
}
//End fnCopyGame
function fnEncodeBoard($stackRows, $gameRow) {
	//takes a set of game stacks and encodes them as a board for stoarge in a player table
	//board is a string of stackNum, ownerPNum and stackHeight delimited by 'x'. This ie repeated for every stack on the board
	fnLogMessageToDb('start of fnEncodeBoard');
	$board = "";

	for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
		for ($stepNum = 0; $stepNum <= $gameRow['AreaSize'] + 1; $stepNum++) {
			//construct index
			$stackRowsIndex = intval('8' . $trackNum . $stepNum);
			$board .= $stackRows[$stackRowsIndex]['StackNum'].'x'.$stackRows[$stackRowsIndex]['OwnerPNum'].'x'.$stackRows[$stackRowsIndex]['StackHeight'].'x';
		}
	}
	//trim trailing 'x'
	$board = substr($board,0,-1);

	fnLogMessageToDb('exiting fnEncodeBoard. resp ='.$board);
	return $board;
}
//End fnEncodeBoard
function fnGetMoves($gameKey, $gameRow, $stackRows, $quick=False) {
	/*
	get all the possible moves for the given game for the next player. if quick is True then stop after first move is found
	return the result as a list (array) of moves in the form (GameRecID, MoveDir, MovePos)
	MoveDir	= direction where 1 is left, 2 is forward and 3 is right
	MovePos = stack number where move starts
	*/

	fnLogMessageToDb ("start fnGetMoves. GK= {$gameKey}");
	global $logLevel;

	//an array to hold all the possible moves
	$movesList = array();
	$errCode=""; $errMsg="";


	if (is_array($gameRow)) {
		//game and stacks data has been provided so don't need to do anything
	}
	else {
		//find out if gameKey is GameRecID or Nmx hash key
		if (strlen($gameKey)<20) {
			$gameRecID=$gameKey; }
		else {
			$gameRecID = 0;
			$hashKey = $gameKey; }

		if ($gameRecID>0) {
			//get game record using GameRecID
			fnLogMessageToDb("get game record using GameRecID");
			$gameRow = fnQrySelectGameByID($gameRecID);
			if (is_string($gameRow)) {
				$errCode = "GM-4";
				$errMsg = $gameRow;
				fnLogMessageToDb ($errCode.$errMsg); }
		}
		else {
			//get the relevant game record using hash key
			fnLogMessageToDb("get game record using hashKey");
			$gameRow = fnQrySelectNmxGame($hashKey);
			if (is_string($gameRow)) {
				$errCode = "GM-5";
				$errMsg = $gameRow;
				fnLogMessageToDb ($errCode.$errMsg); }
		}

		if (strlen($errCode)==0){
			//check player 2 has been set up
			if ($gameRow['P2RecID']==0) {
				$errMsg = "Game " . $gameKey . " not started";
				$errCode = "GM-2";
				fnLogMessageToDb($errCode.$errMsg); }
			else {
				if ($gameRecID>0) {
					//get stack records using rec id
					$stackRows = fnQrySelectStacks($gameRow['GameRef'],$gameRow['TurnCount']);
					if (is_string($stackRows)) {
						$errMsg = "stacks not found. Fatal error.";
						$errCode = "GM-3";
						fnLogMessageToDb($errCode.$errMsg); }
				}
				else {
					//get stacks record using hashkey
					$stackRows = fnQrySelectNmxStacks($hashKey);
					if (is_string($stackRows)) {
						$errMsg = "Stack not found";
						$errCode = "GM-6";
						fnLogMessageToDb($errCode.$errMsg); }
				}
			}
		}
	}

	if (strlen($errCode)==0) {
		//loop to find all the possible moves from the reserve
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//P2's turn.
			//P2's reserves are the ones where the stepNum is AreaSize+1
			$stepNum = $gameRow['AreaSize']+1;
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//construct index
				$stackRowsIndex = intval('8' . $trackNum . $stepNum);
				//only check for moves if the reserve is not empty
				if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
					//check if there is move going left, forward or right
					for ($moveDir = 1; $moveDir < 4; $moveDir++) {
						if (($trackNum == 0 and $moveDir > 1) or
								($trackNum > 0 and $trackNum < $gameRow['AreaWidth'] - 1) or
								($trackNum == $gameRow['AreaWidth'] - 1 and $moveDir < 3)) {
							$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 2);
							//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
							if ($fnResp[0] == '-2'){
								//error in function
								$errCode = "GM-7";
								$errMsg = $fnResp[1].$fnResp[2]; }
							elseif (intval($fnResp[0]) > -1) {
								array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));
								if ($logLevel>3) {fnLogMessageToDb("P2 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
							}
						}
					}
				}
			}
		}
		else {
			//P1's turn.
			//P1's reserves are the ones where the stepNum 0
			$stepNum = 0;
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//construct index
				$stackRowsIndex = intval('8' . $trackNum . $stepNum);
				//only check for moves if the reserve is not empty
				if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
					for ($moveDir = 1; $moveDir < 4; $moveDir++) {
						if (($trackNum == 0 and $moveDir < 3) or
								($trackNum > 0 and $trackNum < $gameRow['AreaWidth'] - 1) or
								($trackNum == $gameRow['AreaWidth'] - 1 and $moveDir > 1)) {
							$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 1);
							//echo "fnFindNextStack resp ".intval($fnResp[0])."<BR>";
							if ($fnResp[0] == -2){
								//error in function
								$errCode = "GM-8";
								$errMsg = $fnResp[1].$fnResp[2]; }
							elseif (intval($fnResp[0]) > -1) {
								array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));
								if ($logLevel>2) {fnLogMessageToDb("P1 possible reserve move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
							}
						}
					}
				}
			}
		}
	}
	//echo "errcode after reserve checks=".$errCode."<"."<BR>";
	if (strlen($errCode)==0){
		//loop to find all the possible moves in the playing area
		if ($gameRow['LastPlayerID'] == $gameRow['P1RecID']) {
			//P2's turn
			//check all the the tracks
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//check each step in the playing area (skip the reserve stacks)
				for ($stepNum = 1; $stepNum < $gameRow['AreaSize'] + 1; $stepNum++) {
					//construct index
					$stackRowsIndex = intval('8' . $trackNum . $stepNum);
					//check stack belongs to P2 otherwise do nothing
					if ($stackRows[$stackRowsIndex]['OwnerPNum'] == 2) {
						//check if there is move going left, forward or right
						for ($moveDir = 1; $moveDir < 4; $moveDir++) {
							if (($trackNum == 0 and $moveDir > 1) or
									($trackNum > 0 and $trackNum < $gameRow['AreaWidth'] - 1) or
									($trackNum == $gameRow['AreaWidth'] - 1 and $moveDir < 3)) {
								$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 2);
								//echo "fnFindNextStack resp ".$fnResp[0]."<BR>";
								if ($fnResp[0] == -2){
									//error in function
									$errCode = "GM-9";
									$errMsg = $fnResp[1].$fnResp[2]; }
								elseif (intval($fnResp[0]) > -1) {
									array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));
									if ($logLevel>4) {fnLogMessageToDb("P2 possible move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
								}
							}
						}
					}
				}
			}
		}
		else {
			//echo "P1"."<BR>";
			//P1's turn
			//check all the the tracks
			for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
				//check each step in the playing area (skip the reserve stacks)
				for ($stepNum = 1; $stepNum < $gameRow['AreaSize'] + 1; $stepNum++) {
					//construct index
					$stackRowsIndex = intval('8' . $trackNum . $stepNum);
					//check stack belongs to P1 otherwise do nothing
					if ($stackRows[$stackRowsIndex]['OwnerPNum'] == 1) {
						//check if there is move going left, forward or right
						for ($moveDir = 1; $moveDir < 4; $moveDir++) {
							if (($trackNum == 0 and $moveDir < 3) or
									($trackNum > 0 and $trackNum < $gameRow['AreaWidth'] - 1) or
									($trackNum == $gameRow['AreaWidth'] - 1 and $moveDir > 1)) {
								$fnResp = fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, 1);
								//echo "fnFindNextStack resp ".$fnResp[0]."<BR>";
								if ($fnResp[0] == -2){
									//error in function
									$errCode = "GM-A";
									$errMsg = $fnResp[1].$fnResp[2]; }
								elseif (intval($fnResp[0]) > -1) {
									array_push($movesList, array($gameKey, $moveDir, $stackRows[$stackRowsIndex]['StackNum']));
									if ($logLevel>4) {fnLogMessageToDb("P2 possible move: MD={$moveDir} MP={$stackRows[$stackRowsIndex]['StackNum']}"); }
								}
							}
						}
					}
				}
			}
		}
	}

	if (strlen($errCode)==0){
		fnLogMessageToDb('fnGetMoves ending. Moves found=' . count($movesList) . '. Response=' . print_r($movesList,True));
		return $movesList; }
	else {
		fnLogMessageToDb('fnGetMoves ending. Response='.$errCode.$errMsg);
		return $errCode.$errMsg; }
}
//End fnGetMoves
function fnPriorityMove($gameRow, $stackRows, $player1or2) {
    fnLogMessageToDb('start fnPriorityMove');
    /* check to see if player has any priority moves available
    priority moves are 1. complete boost and 2. block opponent's boost
    return move position (stackNum) and direction or zero if no move available
    */
    $resp = [0,0];
    //boost requires player to own stack in front of opponent's reserve
    if ($player1or2 == 1) {
        //P1 so check stacks on rhs
		$stepNum = $gameRow['AreaSize']; }
    else {
        //P2 so check stacks on lhs
        $stepNum = 1; }
    $boostStackHeight = 0;
	for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
		//construct index
		$stackRowsIndex = intval('8' . $trackNum . $stepNum);
		if ($stackRows[$stackRowsIndex]['OwnerPNum'] == $player1or2) {
			if ($stackRows[$stackRowsIndex]['StackHeight'] > $boostStackHeight) {
                $resp[0] = $stackRowsIndex;
                $resp[1] = 2;
                $boostStackHeight = $stackRows[$stackRowsIndex]['StackHeight'];
            }
        }
    }

    //only continue if boost not found
    if ($resp[0] == 0) {
        //check if opponent has a boost that can be blocked
        if ($player1or2 == 1) {
            //P1 so check for opponent boosts on lhs
            $stepNum = 1;
            $reserveStep = 0;
        }
        else {
            //P2 so check for opponent boosts on rhs
            $stepNum = $gameRow['AreaSize'];
            $reserveStep = $stepNum + 1;
        }
        $boostStackHeight = 0;
	    for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
            //construct index
            $stackRowsIndex = intval('8' . $trackNum . $stepNum);
            if ($stackRows[$stackRowsIndex]['OwnerPNum'] != $player1or2
               and $stackRows[$stackRowsIndex]['StackHeight'] > $boostStackHeight) {
                //opponent owns this stack so check if it can be blocked
                //try to block using reserve
                $reserveStackIndex = intval('8' . $trackNum . $reserveStep);
                if ($stackRows[$reserveStackIndex]['StackHeight'] >= $stackRows[$stackRowsIndex]['StackHeight']) {
                    $resp[0] = $reserveStackIndex;
                    $resp[1] = 2;
                    $boostStackHeight = $stackRows[$stackRowsIndex]['StackHeight'];
                }
                if ($resp[0] == 0) {
                    //no block yet so try to block using stack in track above
                    if ($trackNum < $gameRow['AreaWidth'] - 1) {
                        $nextStackRowsIndex = intval('8' . $trackNum + 1 . $stepNum);
                        if ($stackRows[$nextStackRowsIndex]['OwnerPNum'] == $player1or2
                           and $stackRows[$nextStackRowsIndex]['StackHeight'] >= $stackRows[$stackRowsIndex]['StackHeight']) {
                            $resp[0] = $nextStackRowsIndex;
                            if ($player1or2 == 1) {
                                $resp[1] = 3; }
                            else {
                                $resp[1] = 1; }
                            $boostStackHeight = $stackRows[$stackRowsIndex]['StackHeight'];
                        }
                    }
                }
                if ($resp[0] == 0) {
                    //no block yet so try to block using stack in track below
                    if ($trackNum > 0) {
                        $nextStackRowsIndex = intval('8' . $trackNum - 1 . $stepNum);
                        if ($stackRows[$nextStackRowsIndex]['OwnerPNum'] == $player1or2
                           and $stackRows[$nextStackRowsIndex]['StackHeight'] >= $stackRows[$stackRowsIndex]['StackHeight']) {
                            $resp[0] = $nextStackRowsIndex;
                            if ($player1or2 == 1) {
                                $resp[1] = 1; }
                            else {
                                $resp[1] = 3; }
                            $boostStackHeight = $stackRows[$stackRowsIndex]['StackHeight'];
                        }
                    }
				}
            }
        }
    }

    fnLogMessageToDb('finished fnPriorityMove. return SRI=' . $resp[0] . '< MD=' . $resp[1]);
    return $resp;
}
//End of fnPriorityMove
function fnFindNextStack($gameRow, $stackRows, $stackRowsIndex, $moveDir, $player1or2) {
	global $logLevel;
	fnLogMessageToDb('(' . $logLevel . ') start fnFindNextStack. SRI=' . $stackRowsIndex . ' MD=' .	$moveDir . ' P1or2=' . $player1or2);
	/* checks to see if it is possible to move in a particular directions
	stackRows = list of all the stacks in the game area (including reserves)
	stackRowsIndex = index of the stack where the move is to start from
	moveDir = direction of the move from the player's perspective. 1 = left, 2 = forward, 3 = right
	player1or2 = player to take move. check that stack belongs to this player otherwise return an error

	returns an array with 3 parts:
	[0] = -2 if there is an error, -1 if move not possible or zero if a move is possible
	[1] = error code or empty string
	[2] = error message or empty string
	*/

	//set up response value
	$fnResp = array("0","","");

	//step num is third and any subsequent digits of the stack num
	$currStepNum = intval(substr((string)$stackRows[$stackRowsIndex]['StackNum'],2,strlen($stackRows[$stackRowsIndex]['StackNum'])-2));
	//echo "<br>"." CS:".$currStepNum;
	//track is second digit of the stack num
	$currTrackNum = intval(substr((string)$stackRows[$stackRowsIndex]['StackNum'],1,1));
	//echo " CT:".$currTrackNum." MD".$moveDir."  ";

	//check player num
	if ($stackRows[$stackRowsIndex]['OwnerPNum'] != $player1or2) {
		$fnResp[0] = '-2';
		$fnResp[1] = 'FN-1';
		$fnResp[2] = 'Stack '. $stackRows[$stackRowsIndex]['StackNum'] . ' does not belong to player ' . $player1or2 . '(' . $stackRows[$stackRowsIndex]['OwnerPNum'] . ')';
		fnLogMessageToDb($fnResp[1].$fnResp[2]);
	}

	//check if move direction is valid (ie. that it does not go outside of the game area)
	if ($fnResp[0] == '0') {
		if ($player1or2 == 1) {
			// P1 moves left to right so first track (track 0) cannot move right and last track cannot move left
			if (	($moveDir == 3 and $currTrackNum == 0) or
					($moveDir == 1 and $currTrackNum == $gameRow['AreaWidth'] - 1 )) {
				$fnResp[0] = '-1';
				$fnResp[1] = 'FN-2';
				$fnResp[2] = 'Move direction ' . $moveDir . ' is invalid for P' . $player1or2 . ' from ' . $stackRowsIndex;
				if ($logLevel>4) fnLogMessageToDb($fnResp[1].$fnResp[2]);
			}
		}
		else {
			// P2 moves right to left so first track (track 0) cannot move left and last track cannot move right
			if (	($moveDir == 1 and $currTrackNum == 0) or
					($moveDir == 3 and $currTrackNum == $gameRow['AreaWidth'] - 1 )) {
				$fnResp[0] = '-1';
				$fnResp[1] = 'FN-3';
				$fnResp[2] = 'Move direction ' . $moveDir . ' is invalid for P' . $player1or2 . ' from ' . $stackRowsIndex;
				if ($logLevel>4) fnLogMessageToDb($fnResp[1].$fnResp[2]);
			}
		}
	}

	//check that next stack is same height or lower
	if ($fnResp[0] == '0') {
		//calculate stackNum of next stack
		$boostMove = False;
		if ($player1or2 == 2) {
			//player 2's move
			if ($moveDir == 3) {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == 1) {
					//boost move
					$nextStepNum = $gameRow['AreaSize'] + 1;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just decrement stackNum
					$nextStepNum = $currStepNum - 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
		}
		else {
			//player 1's move
			if ($moveDir == 3) {
				//decrement trackNum
				$nextTrackNum = $currTrackNum - 1;
				$nextStepNum = $currStepNum;
			}
			elseif ($moveDir == 2) {
				if ($currStepNum == $gameRow['AreaSize']) {
					//boost move
					$nextStepNum = 0;
					$nextTrackNum = $currTrackNum;
					$boostMove = True;
				}
				else {
					//ordinary move - just increment stackNum
					$nextStepNum = $currStepNum + 1;
					$nextTrackNum = $currTrackNum;
				}
			}
			else {
				//increment trackNum
				$nextTrackNum = $currTrackNum + 1;
				$nextStepNum = $currStepNum;
			}
		}
		$nextStackRowsIndex = intval('8' . strval($nextTrackNum) . strval($nextStepNum));

		//don't need to check heights if move is a boost
		if ($boostMove == False) {
			if ($stackRows[$nextStackRowsIndex]['StackHeight'] > $stackRows[$stackRowsIndex]['StackHeight']) {
				$fnResp[0] = '-1';
				$fnResp[1] = 'FN-4';
				$fnResp[2] = 'P' . $player1or2 . ' does not have enough height to make the move from ' . $stackRowsIndex . ' to ' . $nextStackRowsIndex;
				if ($logLevel>4) fnLogMessageToDb($fnResp[1].$fnResp[2]);
			}
		}
	}

	if ($fnResp[0] == 0) {
		if ($logLevel>4) fnLogMessageToDb('fnFindNextStack ending - valid move.');
	}
	return $fnResp;
}
//End fnFindNextStack
function fnCheckLoser($player1or2, $stackRows, $gameRow) {
	fnLogMessageToDb("start fnCheckLoser");
	//returns True if the given player has lost otherwise False
	$fnResp = True;
	//not a loser if there is something in reserve
	if ($player1or2 == 1) {
		//check P1 reserves
		$stepNum = 0;
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//construct index
			$stackRowsIndex = intval('8' . $trackNum . $stepNum);
			if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
				 $fnResp = False; }
		}
	}
	else {
		//check P2 reserves
		$stepNum = $gameRow['AreaSize']+1;
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//construct index
			$stackRowsIndex = intval('8' . $trackNum . $stepNum);
			if ($stackRows[$stackRowsIndex]['StackHeight'] > 0) {
				$fnResp = False;
				break; }
		}
	}

	if ($fnResp) {
		//not a loser if player has a counter on top of one of the game steps
		//check all the the tracks
		for ($trackNum = 0; $trackNum < $gameRow['AreaWidth']; $trackNum++) {
			//check each step in the playing area (skip the reserve stacks)
			for ($stepNum = 1; $stepNum < $gameRow['AreaSize'] + 1; $stepNum++) {
				//construct index
				$stackRowsIndex = intval('8' . $trackNum . $stepNum);
				if ($player1or2 == $stackRows[$stackRowsIndex]['OwnerPNum']) {
					$fnResp = False;
					break(2); }
			}
		}
	}

	if ($fnResp) {
		fnLogMessageToDb('ending fnCheckLoser. Response = True'); }
	else {
		fnLogMessageToDb('ending fnCheckLoser. Response = False'); }

	return $fnResp;
}
//End fnCheckLoser
function fnProcessInput($callString) {
	//globals
	global $tbP1NewPlyrName, $tbP1NewGameRef;
	global $tbP2NewPlyrName, $tbP2NewGameRef;
	global $conn, $logLevel;
	fnLogMessageToDb('(' . $logLevel . ') start fnProcessInput');

	//convert call string to upper case
	$callString = strtoupper($callString);

	//call function to extract the parameters from the call string
	$refs=fnFindParams($callString);
	//print_r($refs);

	$errCode = ""; $errMsg = "";
	if (array_key_exists('errCode',$refs) == True) {
		$errCode = $refs['errCode'] ;
        $errMsg = $refs['errMsg'] ; }
	elseif (array_key_exists("TYPE",$refs) == False) {
		$errCode = "PrI-1";
		$errMsg = "input string - type param missing"; }#
    else {
		switch ($refs["TYPE"]){
			case "TESTING":
				/*
				//echo 'logLevel=' . $logLevel . '<' . '<BR>';
				//$errMsg = fnProcessMove ($refs['GR'], '', '', $refs['MD'], $refs['SR']);
				if (array_key_exists('GAMEREF',$refs) == True) {
					$fnResp = fnRunGCP2($refs['PLAYERNAME'], $refs['GAMEREF']); }
				else {
					$fnResp = fnRunGCP2($refs['PLAYERNAME'], 0); }

				if ($fnResp == '') {
					//successful move so return state of play
					$fnResp = fnStateOfPlay($refs['GAMEREF']);
					if (is_array($fnResp)) {
						//all ok so send stats of play data back
						$errCode = "    ";
						$errMsg = json_encode($fnResp); }
					else {
						//error from state of play
						$errCode = substr($fnResp,0, 5);
						$errMsg = substr($fnResp,5, strlen($fnResp)- 5); }
				}
				else {
					//error from state of play
					$errCode = substr($fnResp,0, 5);
					$errMsg = substr($fnResp,5, strlen($fnResp)- 5);
				}
				*/
				//fnSimulateMove($gameState, &$xGameRow, &$xStackRows, $moveDir, $movePos, $newGameState)
				$gameState = '800x1x5x801x0x0x802x0x0x803x0x0x804x0x0x805x0x0x806x2x5x810x1x5x811x0x0x812x0x0x813x0x0x814x0x0x815x0x0x816x2x5';
				$xGameRow = [];
				$xStackRows = [];
				fnRollout($gameState, $xGameRow, $xStackRows);
				break;

			case 'COPY':
			case 'COPYGAME':
				print(fnCopyGame($refs['GAMEREF'], $refs['TURNCOUNT']));
				break;

			case "GET":
			case "GETMOVES":
				print_r(fnGetMoves($refs["GAMEREF"],[],[]));
				break;

			case 'DC':
			case "DATACHECK":
			case "CHECK":
				fnDataChecks($refs["GAMEREF"]);
				break;

			case 'P1N':
			case "P1_NEW_GAME":
			case "P1NEWGAME":
				//player name and game central are both optional parameters
                if (count($refs) > 3) {
                    //error - too many parameters
					$errCode = "P1N-1";
					$errMsg = "P1 New Player - too many parameters";}
				elseif (count($refs) == 1) {
					//error - no parameters
					$errCode = "P1N-4";
					$errMsg ="P1 New Player - no parameters"; }
				elseif (count($refs) == 2) {
					//if there is one parameter then it must be Player Name
                    if (array_key_exists("PLAYERNAME",$refs) == False ) {
                        //error - only valid parameter is player name
						$errCode = "P1N-2";
                        $errMsg = "P1 New Player - invalid parameters"; } }
				elseif ((array_key_exists("PLAYERNAME",$refs) == False ) or (array_key_exists("GAMECENTRAL",$refs) == False )) {
					//error - invalid parameters
					$errCode = "P1N-3";
					$errMsg ="P1 New Player - invalid parameters"; }
				elseif (array_key_exists("GAMECENTRAL",$refs) == True And $refs["GAMECENTRAL"] <> "Y" And $refs["GAMECENTRAL"] <>  "N") {
					//error game central must by y or n
					$errCode = "P1N-8";
					$errMsg ="P1 New Player - game central parameter must by y or n"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					if (count($refs) == 2) {
						//one parameter which must be the player name. Set gameCentral to No
                        $gameCentral = "N";
						$tbP1NewPlyrName = $refs["PLAYERNAME"]; }
					else {
						//two parameters for both game central and player name
						fnLogMessageToDb("P1 new game params - player name =" . $refs["PLAYERNAME"] . " and game ref =" . $refs["GAMECENTRAL"]);
						$gameCentral = $refs["GAMECENTRAL"];
						$tbP1NewPlyrName = $refs["PLAYERNAME"];}

					//check the password
					$fnResp = fnValidatePlayerNamePW($tbP1NewPlyrName);
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = 'P1N-7';
						$errMsg = $fnResp; }
					else {
						//extract player name
						$inx = strpos($tbP1NewPlyrName,"/",0);
						$tbP1NewPlyrName = substr($tbP1NewPlyrName,0,$inx); }
				}
				if (strlen($errCode) == 0) {
					$fnResp = fnProcessNewP1($tbP1NewPlyrName, True);
					if (array_key_exists('errCode',$fnResp) == False) {
                        //Response from fnProcessNewP1 is ok - a numeric fnResp means the new game ref has been returned instead of string error message
						$gameRef = $fnResp['gameRef'];
						//call state of play
						$fnResp = fnStateOfPlay($gameRef);
						if (is_array($fnResp)) {
							$errCode = "    ";
                            $errMsg = json_encode($fnResp); }
                        else {
							$errCode = 'P1N-5';
                            //$errMsg = "P1 New Player - error response from state of play"; } }
							$errMsg = $fnResp; }
					}
					else {
						//error occurred in fnProcessNewP1
                        $errCode = 'P1N-6';
                        //$errMsg = "P1 New Player - error response from process new player 1"; }
						$errMsg = $fnResp['errCode'] . $fnResp['errMsg']; }

					//if state of play has been called successfully then check GAMECENTRAL flag and set up player 2 if required
					if ($errCode == "    " and $gameCentral == "Y") {
						$fnResp = fnProcessNewP2($gameRef, "GAMECENTRAL", True);
						if (strlen($fnResp) > 0) {
							//errors occurred
							$errCode = substr($fnResp,0, 5);
							$errMsg = "error occurred setting up GAMECENTRAL as player 2 [" . substr($fnResp,5,strlen($fnResp)-5) . "]"; }
						else {
							//call state of play
							$fnResp = fnStateOfPlay($gameRef);
							if (is_array($fnResp)) {
								$errCode = "    ";
								$errMsg = json_encode($fnResp); }
							else {
								$errCode = "P1N-9";
								$errMsg = "P1 New Player - error response from state of play (for player 2)"; }
						}
					}
				}
				break;

			case 'P2N':
			case "P2_NEW_GAME":
			case "P2NEWGAME":
                //must have parameters for gameRef and playerName
                if (count($refs) != 3) {
					//error - incorrect number of parameters
                    $errCode = "P2N-1";
                    $errMsg = "P2 New Player - incorrect number of parameters"; }
                else {
					//2 parameters - one must be game ref and the other player name.
					if (array_key_exists("GAMEREF",$refs) == False) {
						//error - game ref parameter missing
						$errCode = "P2N-2";
						$errMsg = "P2 New Player - game ref parameter missing"; }
					elseif (array_key_exists("PLAYERNAME",$refs) == False) {
						//error - player name parameter missing
						$errCode = "P2N-3";
						$errMsg = "P2 New Player - the player name parameter is missing"; }
					elseif (!preg_match('/^[0-9]+$/',$refs["GAMEREF"])) {
						//game ref must be numeric
						$errCode = "P2N-4";
						$errMsg = "P2 New Player - game ref not numeric"; }
				}

				if (strlen($errCode) == 0) {
					//valid input so carry on
					$tbP2NewGameRef = $refs["GAMEREF"];
					$tbP2NewPlyrName = $refs["PLAYERNAME"];

					//check the password
					$fnResp = fnValidatePlayerNamePW($tbP2NewPlyrName);
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = 'P2N-7';
						$errMsg = $fnResp; }
					else {
						//extract player name
						$inx = strpos($tbP2NewPlyrName,"/",0);
						$tbP2NewPlyrName = substr($tbP2NewPlyrName,0,$inx); }
				}

				if (strlen($errCode) == 0) {
					//all ok so create player 2
					$fnResp = fnProcessNewP2($tbP2NewGameRef, $tbP2NewPlyrName, True);
                    //if (strlen($fnResp) == 0) {
					if (array_key_exists('errCode',$fnResp) == False) {
                        //all ok
                        //call state of play and return state of play message
                        $tbP1NewGameRef = $tbP2NewGameRef;
                        $fnResp = fnStateOfPlay($tbP1NewGameRef);

						if (is_array($fnResp)) {
                            $errCode = "    ";
                            $errMsg = json_encode($fnResp);}
                        else {
                            $errCode = "P2N-9";
                            $errMsg = "P2 New Player - error response from state of play";} }
                    else {
                        //errors occurred setting up player 2
                        $errCode = 'P2N-8,'.$fnResp['errCode'];;
						$errMsg = $fnResp['errMsg'];}
				}
				break;

			case 'GS':
			case "GAME_STATUS":
			case "GAMESTATUS":
				//set type to a standard value
				$refs["TYPE"] = 'GAMES_STATUS';
				$turnCount=0;
				//must have parameters for gameRef; turn count is optional
				if (count($refs) > 3) {
					//error - incorrect number of parameters
					$errCode = "GS-1";
					$errMsg = "Game Status - incorrect number of parameters"; }
				elseif (array_key_exists("GAMEREF",$refs)==False) {
					//error - game ref parameter missing
					$errCode = "GS-2";
					$errMsg = "Game Status - game ref parameter missing"; }
				elseif (count($refs) == 3) {
					if (array_key_exists("TURNCOUNT",$refs)==False) {
						//error - turn count parameter missing
						$errCode = "GS-4";
						$errMsg = "Game Status - turn count parameter missing"; }
					else {$turnCount = $refs["TURNCOUNT"];}
				}
				if (strlen($errCode) == 0) {
					//valid input so carry on
					$gameRef = $refs["GAMEREF"];
					$fnResp = fnStateOfPlay($gameRef,$turnCount);
					if (is_array($fnResp)) {
						$errCode = "    ";
						$errMsg = json_encode($fnResp);}
					else {
						$errCode = substr($fnResp,0,5);
						$errMsg = substr($fnResp,5,strlen($fnResp)-5);}
				}
				break;

			case 'P1M':
			case 'P2M':
			case "P1_MOVE":
			case "P1_MOVES":
			case "P1MOVE":
			case "P1MOVES":
			case "P2MOVES":
			case "P2MOVE":
			case "P2_MOVE":
			case "P2_MOVES":
				if (substr($refs['TYPE'],0,2) == 'P1') {
					$refs['TYPE'] = 'P1_MOVE';
					$p1or2Move = 1; }
				if (substr($refs['TYPE'],0,2) == 'P2') {
					$refs['TYPE'] = 'P2_MOVE';
					$p1or2Move = 2; }
				//must have parameters for game ref, player name, move pos and move count.
				if (count($refs) <> 5) {
					//error - incorrect number of parameters
					$errCode = "PM-1";
					$errMsg = "Player Move - incorrect number of parameters"; }
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "PM-2";
					$errMsg = "Player Move - game ref parameter missing";}
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player ref parameter missing
					$errCode = "PM-3";
					$errMsg = "Player Move - player name parameter missing"; }
				elseif (array_key_exists("MOVEPOS",$refs) == False) {
					//error - move position parameter missing
					$errCode = "PM-4";
					$errMsg = "Player Move - move position parameter missing"; }
				elseif (array_key_exists("MOVEDIR",$refs) == False) {
					//error - move direction parameter missing
					$errCode = "PM-5";
					$errMsg = "Player Move - move count/num parameter missing"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					if (!preg_match('/^[0-9]+$/',$refs["GAMEREF"])) {
						//game ref must be numeric
						$errCode = "PM-7c";
						$errMsg = "Player Move - game ref must be numeric"; }
					else {
						$gameRef = $refs["GAMEREF"];
						$fnResp = fnValidatePlayerNamePW($refs["PLAYERNAME"]);
						if (substr($fnResp,0, 3) == 'VP-') {
							$errCode = 'PM-6';
							$errMsg = $fnResp; }
						else {
							$playerRef = $fnResp;

							if (!preg_match('/^[0-9]+$/',$refs['MOVEPOS'])) {
								//move pos must be numeric
								$errCode = 'PM-7a';
								$errMsg = 'Player Move - move pos must be numeric ' . $refs['MOVEPOS'] ; }
							else {
								$movePos = $refs['MOVEPOS'];

								if (!preg_match('/^[1-3]{1}$/',$refs['MOVEDIR'])) {
									//move num must be a 1, 2 or 3
									$errCode = 'PM-7b';
									$errMsg = 'Player Move - move direction must be 1, 2 or 3'; }
								else {
									$moveNum = $refs['MOVEDIR']; }
							}
						}
					}
				}

				if (strlen($errCode) == 0) {
					//input has been put into the form. Now run the play function
					$fnResp = fnValidatePlay($gameRef, $playerRef, $p1or2Move, $movePos, $moveNum);

					if ($fnResp["errCode"]=="" or $fnResp["errCode"]=="PMO-6" or $fnResp["errCode"]=="PMO-6a" or $fnResp["errCode"]=="PMO-4" or $fnResp["errCode"]=="PMO-4a") {
						//successful move so return state of play
						$fnResp = fnStateOfPlay($gameRef);
						if (is_array($fnResp)) {
							//all ok so send stats of play data back
							$errCode = "    ";
							$errMsg = json_encode($fnResp); }
						else {
							//error from state of play
							$errCode = substr($fnResp,0, 5);
							$errMsg = substr($fnResp,5, strlen($fnResp)- 5); } }
					else {
						//error from fnValidatePlay
						if ($fnResp["errCode"]=="V") {
							$errCode = "PM-8";
							$errMsg = "Player Move - error from validate play"; }
						else {
							$errCode = "PM-8,".$fnResp["errCode"];
							$errMsg = $fnResp["errMsg"]; } }
				}
				break;

			case 'PI':
			case "PLAYER_INFO":
			case "PLAYERINFO":
				//one parameter which is player name
				if (count($refs) <> 2) {
					//error - incorrect number of parameters
					$errCode = "PI-1";
					$errMsg = "Player info - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - invalid parameter
					$errCode = "PI-2";
					$errMsg = "Player info - requires player name parameter"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					//search the players table to retrieve player ref
					$playerRow = fnQrySelectPlayer(strtoupper(trim($refs["PLAYERNAME"])));
					if (is_string($playerRow)) {
						$errCode = "PI-5";
						$errMsg = "Player info - player name not found"; }
					else {
						$playerRef = $playerRow['PlayerRef'];
						//prepare output
						$errCode = "    ";
						$errMsg = "player_info(playerName=".strtoupper(trim($refs["PLAYERNAME"]));
						//find any games that involve the player
						$gameRows=fnQrySelectGamesForPlayer($playerRef);
						if (is_string($gameRows)) {
							//output already set up and nothing to add except the closing bracket
							$errMsg .= ")";
							$errMsg = json_encode(array("type" => "player_info", "PlayerName" => strtoupper(trim($refs["PLAYERNAME"])), "Games" => array())); }
						else {
							for ($i=0; $i < count($gameRows); $i++) {
								if ($gameRows[$i]['Winner']==0){$gameState="in progress";}
								elseif ($gameRows[$i]['Winner']==$playerRef){$gameState="won";}
								else {$gameState="lost";}
								$errMsg .= ";Game=".$gameRows[$i]['GameRef'].",GameState=".$gameState;
								$games[] = array("GameRef" => $gameRows[$i]['GameRef'], "AreaSize" => $gameRows[$i]['AreaSize'], "GameState" => $gameState); }
							$errMsg .= ")";
							$errMsg = json_encode(array("type" => "player_info", "PlayerName" => strtoupper(trim($refs["PLAYERNAME"])), "Games"=> $games)) ; }
					}
				}
				break;

			case 'OG':
			case "OPEN_GAMES":
			case "OPENGAMES":
				//No parameters
				if (count($refs) <> 1) {
					//error - incorrect number of parameters
					$errCode = "OG-1";
					$errMsg = "Open Games - incorrect number of parameters"; }

				if (strlen($errCode) == 0) {
					//valid input so carry on
					//select all games where P2RecID = 0 and join with players table to get player 1 PlayerName
					$openGamesRows = fnQrySelectOpenGames() ;
					if (is_string($openGamesRows)) {
						$errCode = "    ";
						$errMsg = "Open_Games(none)";
						$errMsg = json_encode(array("type" => "Open_Games","Games" => array()));}
					else {
						$errCode = "    ";
						$errMsg = "Open_Games(";
						for ($i=0; $i < count($openGamesRows); $i++) {
							$games[] = array("GameRef" => $openGamesRows[$i]['GameRecID'], "Player1Name" => $openGamesRows[$i]['PlayerName']);
							$errMsg = $errMsg."Game=".$openGamesRows[$i]['GameRecID']. "," . $openGamesRows[$i]['PlayerName'] . ";" ; }
						$errMsg = $errMsg.")"	;
						$errMsg = json_encode(array("type" => "Open_Games","Games" => $games)); }
				}
				break;

			case 'REG':
			case "REGISTER":
				//two parameters
				if (count($refs) <> 3) {
					//error - incorrect number of parameters
					$errCode = "RP-1";
					$errMsg = "Register Player - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "RP-2";
					$errMsg = "Register Player - player name parameter missing";}
				elseif (array_key_exists("PIN",$refs) == False) {
					//error - PIN parameter missing
					$errCode = "RP-3";
					$errMsg = "Register Player - PIN parameter missing";}
				else {
					$password = $refs["PIN"];
					$playerName = $refs["PLAYERNAME"];

					//PIN cannot be blank
					if (strlen($password)<4 or !ctype_digit($password)) {
						$errCode = "RP-5";
						$errMsg = "Register Player - PIN must be 4 or more digits";}
					else {
						//player name cannot be blank
						if (strlen($playerName)==0) {
							$errCode = "RP-6";
							$errMsg = "Register Player - player name cannot be blank";}
						//player name comprise letters and numbers only
						elseif (!preg_match('/^[a-zA-Z0-9 ]+$/',$playerName)) {
							$errCode = "RP-6";
							$errMsg = "Register Player - player name - " . $playerName . " - must contain only letters and numbers";}
						else {
							//make sure player name doesn't exist already
							$fnResp = fnQrySelectPlayer($playerName);
							if ($fnResp != "player not found") {
								$errCode = "RP-6";
								$errMsg = "Register Player - player name already exists";}
						}
					}
				}

				if (strlen($errCode) == 0) {
					//valid input so carry on
					//get a new player ref
					$refsRow = fnReferencesTable("new player ref");
					if (is_string($refsRow)) {
						$errCode = "RP-8";
						$errMsg= "Error setting up new player ref"; }
					else {
						$playerRef = $refsRow['NextPlayerRef']; }
					//create new player record
					$fnResp = fnQryAddPlayer($playerRef,$playerName,$password,$hint);
					if (strlen($fnResp) > 0) {
						$errCode = "RP-7";
						$errMsg = "Register Player - error in fnQryAddPlayer [$fnResp]";}
					else {
						$errCode = "    ";
						$errMsg = "register(playername=".$playerName.";PIN=".$password;
						$errMsg = json_encode(array("type" => "register","playername" => $playerName, "PIN" => $password)); }
				}
				break;

			case 'RES':
			case "RESIGN":
				//two parameters
				if (count($refs) <> 3) {
					//error - incorrect number of parameters
					$errCode = "PR-1";
					$errMsg = "Player Resigns - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "PR-2";
					$errMsg = "Player Resigns - player name parameter missing";}
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "PR-3";
					$errMsg = "Player Resigns - game ref parameter missing";}

				if (strlen($errCode) == 0) {
					//valid input so carry on
					$gameRef = $refs["GAMEREF"];
					$playerName = $refs["PLAYERNAME"];
					//check player name/PIN
					$fnResp = fnValidatePlayerNamePW($playerName);
					//if (!ctype_digit($fnResp)) {
					if (substr($fnResp,0, 3) == 'VP-') {
						$errCode = "PR-7";
						$errMsg = $fnResp; }
					else {
						//Update game record
						$playerRef = $fnResp;
						$gameRow = fnQrySelectGame($gameRef);
						if (is_string($gameRow)) {
							$errCode = "PR-4";
							$errMsg = "Player Resigns - error retrieving game record";}
						else {
							//check player is part of game
							if ($playerRef != $gameRow['P1RecID'] And $playerRef != $gameRow['P2RecID']) {
								$errCode = "PR-5";
								$errMsg = "Player Resigns - player is not part of game";
								fnLogMessageToDb($errCode.$errMsg."/".$playerRef."/".$gameRow['P1RecID']."/".$gameRow['P2RecID']);}
							//check game hasn't already finished
							elseif ($gameRow['Winner'] != 0) {
								$errCode = "PR-6";
								$errMsg = "Player Resigns - game is already finished";
								fnLogMessageToDb($errCode.$errMsg."/".$gameRow['Winner']);}
							elseif ($gameRow['TurnCount'] >= 10) {
								//set a winner and save game
								if ($playerRef == $gameRow['P1RecID']) {
									$gameRow['Winner'] = $gameRow['P2RecID'] ;}
								else {
									$gameRow['Winner'] = $gameRow['P1RecID'] ;}
								$fnResp = fnQryUpdateGame($gameRow['GameRecID'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],date("Y-m-d H:i:s"),$gameRow['Winner'],$gameRow['TurnCount']);
								if (strlen($fnResp) > 0) {
									$errCode = "PR-8";
									$errMsg = "Player resigns - ". $fnResp;	}
								else {
									$errCode = "    ";}
							}
							else {
								//set game to void and save game
								$gameRow['Winner'] = -1*$playerRef ;
								$fnResp = fnQryUpdateGame($gameRow['GameRecID'],$gameRow['P1RecID'],$gameRow['P2RecID'],$gameRow['LastPlayerID'],date("Y-m-d H:i:s"),$gameRow['Winner'],$gameRow['TurnCount']);
								if (strlen($fnResp) > 0) {
									$errCode = "PR-9";
									$errMsg = "Player resigns - ". $fnResp;	}
								else {
									$errCode = "    "; }
							}
							//if there are no errors then return the state of play message
							if ($errCode == "    ") {
								$fnResp = fnStateOfPlay($gameRef);
								if (is_array($fnResp)) {
									//all ok so send stats of play data back
									$errCode = "    ";
									$errMsg = json_encode($fnResp); }
								else {
									//error from state of play
									$errCode = substr($fnResp,0, 5);
									$errMsg = substr($fnResp,5, strlen($fnResp)- 5); }
							}
						}
					}
				}
				break;

			case "GO_BACK":
			case "GOBACK":
				//revert to an earlier stage in the game. Only GAMECENTRAL can use this
				//three parameters - playername, game ref and turncount
				fnLogMessageToDb("Go back message");
				if (count($refs) <> 4) {
					//error - incorrect number of parameters
					$errCode = "GB-1";
					$errMsg = "Go back - incorrect number of parameters"; }
				elseif (array_key_exists("PLAYERNAME",$refs) == False) {
					//error - player name parameter missing
					$errCode = "GB-2";
					$errMsg = "Go back - player name parameter missing";}
				elseif (array_key_exists("GAMEREF",$refs) == False) {
					//error - game ref parameter missing
					$errCode = "GB-3";
					$errMsg = "Go back - game ref parameter missing";}
				elseif (array_key_exists("TURNCOUNT",$refs) == False) {
					//error - turn count parameter missing
					$errCode = "GB-4";
					$errMsg = "Go back - turn count parameter missing";}
				else {
					//get game record
					$gameRef = $refs["GAMEREF"];
					$playerName = $refs["PLAYERNAME"];
					$turnCountGB = $refs["TURNCOUNT"];
					$gameRow = fnQrySelectGame($gameRef);
					if (is_string($gameRow)) {
						$errCode = "GB-6";
						$errMsg = "Go back - error retrieving game record";}
					elseif ($turnCountGB < 1 OR $turnCountGB >= $gameRow['TurnCount']) {
						$errCode = "GB-7";
						$errMsg = "Go back - turn count out of range";}
				}
				fnLogMessageToDb("passed go message checks");
				if (strlen($errCode) == 0) {
					//valid input so carry on
					//check player name/PIN is valid and that it is GAMECENTRAL
					$fnResp = fnValidatePlayerNamePW($playerName);
					if (substr($fnResp,0, 3) == 'VP-' OR $fnResp != 1) {
						$errCode = "GB-5";
						$errMsg = $fnResp; }
					else {
						/*
						Overwrite turn count 0 records with data from corresponding record for GOBACK turn count
						then delete every record with a turn count greater than GOBACK
						*/
						//begin database transaction
						fnLogMessageToDb("start transaction");
						$conn->beginTransaction();
						$logMsgs=array("rewrite log msgs after rollback");
						//check for any errors
						if (strlen($errCode) == 0) {
							//all ok so do the updates
							//overwrite Game record for turnCount0 with values from record for turnCount Go BACK
							fnLogMessageToDb("Get game record for turn coutn = GOBACK");
							array_push($logMsgs,"Get game record for turn count = GOBACK");
							$gameRowGB = fnQrySelectGame($gameRef, $turnCountGB);
							if (is_string($gameRowGB)) {
								$errMsg = "Game " . $gameRef . " and turncount ". $turnCountGB . " not found";
								$errCode = "GB-8";
								fnLogMessageToDb($errCode.$errMsg);
								array_push($logMsgs,$errCode.$errMsg); }
							else {
								fnLogMessageToDb("update game record for turn count = 0  with value from turn count = GOBACK");
								array_push($logMsgs,"update game record for turn count = 0  with value from turn count = GOBACK");
								$fnResp = fnQryUpdateGame($gameRowGB['GameRecID'],$gameRowGB['P1RecID'],$gameRowGB['P2RecID'],$gameRowGB['LastPlayerID'],date("Y-m-d H:i:s"),$gameRowGB['Winner'],$gameRowGB['TurnCount'],0);
								if (strlen($fnResp) > 0) {
									$errCode = "GB-9";
									$errMsg = "Go Back - " . $fnResp;
									fnLogMessageToDb($errCode.$errMsg);
									array_push($logMsgs,$errCode.$errMsg); }
								else {
									//delete Game records with turn count > Go Back
									fnLogMessageToDb("delete game records with turn count > GOBACK");
									array_push($logMsgs,"delete game records with turn count > GOBACK");
									$fnResp = fnQryDeleteGames($gameRef, $turnCountGB);
									if (strlen($fnResp) > 0) {
										$errCode = "GB-I";
										$errMsg = "Go Back - " . $fnResp;
										fnLogMessageToDb($errCode.$errMsg);
										array_push($logMsgs,$errCode.$errMsg); }
									else {
										//delete the Stacks records with turn count > Go BACK
										fnLogMessageToDb("delete stacks with turn count > GOBACK");
										array_push($logMsgs,"delete stacks with turn count > GOBACK");
										$fnResp = fnQryDeleteStacksForManyTurns($gameRef, $turnCountGB);
										if (strlen($fnResp) > 0) {
											$errCode = "GB-A";
											$errMsg = "Go Back - " . $fnResp;
											fnLogMessageToDb($errCode.$errMsg);
											array_push($logMsgs,$errCode.$errMsg); }
										else {
											//get the Stacks records with turn count = 0
											fnLogMessageToDb("get stacks with turn count = 0");
											array_push($logMsgs,"get stacks with turn count = 0");
											$stackRows0 = fnQrySelectStacks($gameRef, 0);
											if (is_array($stackRows0)==False) {
												$errCode = "GB-B";
												$errMsg = "Stacks for game " . $gameRef . " and turncount 0 not found";
												fnLogMessageToDb($errCode.$errMsg);
												array_push($logMsgs,$errCode.$errMsg); }
											else {
												//get the Stacks records with turn count = GOBACK
												fnLogMessageToDb("update stacks with turn count = GOBACK");
												array_push($logMsgs,"update stacks with turn count = GOBACK");
												$stackRowsGB = fnQrySelectStacks($gameRef,$turnCountGB);
												if (is_array($stackRowsGB)==False) {
													$errMsg = "Stacks for game " . $gameRef . " and turncount " . $turnCountGB . " not found";
													$errCode = "GB-C";
													fnLogMessageToDb($errCode.$errMsg);
													array_push($logMsgs,$errCode.$errMsg); }
												else {
													//copy the Stacks records with turn count = GO BACK into stacks with turn count = 0
													fnLogMessageToDb("copy the stacks with turn count = GO BACK into the stacks with turn count = 0");
													array_push($logMsgs,"copy the stacks with turn count = GO BACK into the stacks with turn count = 0");
													$maxI = count($stackRowsGB);
													for ($i=0; $i<$maxI; $i++) {
														$fnResp = fnQryUpdateStack($stackRowsGB[$i]['StackNum'],$stackRowsGB[$i]['StackHeight'],$stackRowsGB[$i]['OwnerPNum'],$stackRows0[$i]['StackRecID'],0);
														//fnQryUpdateStack($stackNum, $stackHeight, $top, $stackRecID, $turnCount)
														if (strlen($fnResp) > 0 ){
															$errCode = "GB-D";
															$errMsg = "Go Back - " . $fnResp;
															fnLogMessageToDb($errCode.$errMsg);
															array_push($logMsgs,$errCode.$errMsg);
															break; }
													}
													//if there were no errors then carry on
													if (strlen($errCode) == 0) {
														//delete the counters records with turn count > Go BACK
														fnLogMessageToDb("delete counters with turn count > GO BACK");
														array_push($logMsgs,"delete counters with turn count > GO BACK");
														$fnResp = fnQryDeleteCountersForManyTurns($gameRef, $turnCountGB);
														if (strlen($fnResp) > 0) {
															$errCode = "GB-E";
															$errMsg = "Go Back - " . $fnResp;
															fnLogMessageToDb($errCode.$errMsg);
															array_push($logMsgs,$errCode.$errMsg); }
														else {
															//delete the counters records with turn count = 0
															fnLogMessageToDb("delete counters with turn count = 0");
															array_push($logMsgs,"delete counters with turn count = 0");
															$fnResp = fnQryDeleteCountersForOneTurn($gameRef, 0);
															if (strlen($fnResp) > 0) {
																$errCode = "GB-F";
																$errMsg = "Go Back - " . $fnResp;
																fnLogMessageToDb($errCode.$errMsg);
																array_push($logMsgs,$errCode.$errMsg); }
															else {
																//copy the Counters records with turn count = GO BACK into new Counters records with turn count = 0
																fnLogMessageToDb("select counters");
																$countersRowsGB = fnQrySelectAllCounters($gameRef,$turnCountGB);
																if (is_string($countersRowsGB)) {
																	$errMsg = "Counter records for game " . $gameRef . " and turncount " . $turnCountGB . "not found";
																	$errCode = "GB-G";
																	fnLogMessageToDb($errCode.$errMsg);
																	array_push($logMsgs,$errCode.$errMsg); }
																else {
																	fnLogMessageToDb("copy counters");
																	array_push($logMsgs,"copy counters");
																	for ($i=0; $i<count($countersRowsGB); $i++) {
																		$fnResp = fnQryAddCounter($countersRowsGB[$i]['GameRecID'],$countersRowsGB[$i]['StackNum'],$countersRowsGB[$i]['PlayerRef'],$countersRowsGB[$i]['CounterHeight'], 0);
																		if (strlen($fnResp) > 0 ){
																			$errCode = "GB-H";
																			$errMsg = "Go Back - " . $fnResp;
																			fnLogMessageToDb($errCode.$errMsg);
																			array_push($logMsgs,$errCode.$errMsg);
																			break; }
																	}
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
						if (strlen($errCode) > 0) {
							//an error has occurred rollback any database uopdates
							$conn->rollback();
							//(re)write log messages
							foreach ($logMsgs as $msgs) {fnLogMessageToDb($msgs);}
							fnLogMessageToDb ("Go Back - Rollback"); }
						else {
							$conn->commit();
							$fnResp = fnStateOfPlay($gameRef);
							if (is_array($fnResp)) {
								//all ok so send stats of play data back
								$errCode = "";
								$errMsg = json_encode($fnResp); }
							else {
								//error from state of play
								$errCode = substr($fnResp,0, 5);
								$errMsg = substr($fnResp,5, strlen($fnResp)- 5); }
						}
					}
				}
				break;

			default:
				//error - invalid input
				$errCode = "PrI-2";
				$errMsg = "input type " . $refs["TYPE"] . " not recognised";
		}
	}

	fnLogMessageToDb("(fnProcessInput) status after case statements: " . $errCode.$errMsg);
	if (strlen(trim($errCode))==0){
		return $errMsg;}
	else{
		return json_encode(array("type"=>"error","ErrorCode"=>$errCode,"ErrorMsg"=>$errMsg));}
}
//End fnProcessInput
function fnFindParams($callString) {
 	fnLogMessageToDb("start fnFindParams");

	//decode the json and create an assoicative array
	$refs = json_decode($callString, True);
	if (is_null($refs)) {
		$refs['errCode'] = 'FP-1' ;
		$jsErrMsg = json_last_error() ;
		$refs['errMsg'] = 'Find Params - invalid json - '. $jsErrMsg ;
		fnLogMessageToDb($refs['errCode'].$refs['errMsg']);
		return $refs; }
	else {
		return $refs;}
}
//End fnFindParams
function fnProcessNewP1($tbP1NewPlyrName, $silent, $password="", $hint=""){
 	fnLogMessageToDb("start fnProcessNewP1");
	//globals
	global $tbP1NewGameRef, $tbP1NewPlyrRef;
	global $tbP2NewGameRef, $tbP2NewPlyrRef, $logLevel;

	$errMsg = ""; $errCode = "";
	$gameRef=0;
	$playerRef=0;
	$playerRow=null; $refsRow=null;

	//if player name is not blank then see if it already exists
	if (strlen($tbP1NewPlyrName) > 0) {
		$playerRow = fnQrySelectPlayer(strtoupper(trim($tbP1NewPlyrName)));
		if (is_array($playerRow)) {
			$playerRef = $playerRow['PlayerRef'];}
	}

	//get the next game and player refs
	if ($playerRef > 0) {
		//already have a player ref so just get new game ref
		$refsRow = fnReferencesTable("new game ref");
		if (is_array($refsRow)) {
			$gameRef = $refsRow['NextGameRef'];	}
		else {
			$errMsg = 'New player 1 - error setting up new game reference';
			$errCode = 'NP1-1';
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}
	else {
		$refsRow = fnReferencesTable('new both');
		if  (is_array($refsRow)) {
			$gameRef = $refsRow['NextGameRef'];
			$playerRef = $refsRow['NextPlayerRef']; }
		else {
			$errMsg = 'New player 1 - error setting up new game and player references';
			$errCode = 'NP1-2';
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}

	//if player name is not blank and its not already on record then associate input name with new player
	if (strlen($tbP1NewPlyrName) > 0 and is_array($playerRow)==False) {
		$fnResp = fnQryAddPlayer($playerRef, strtoupper(trim($tbP1NewPlyrName)),$password,$hint);
		if (strlen($fnResp) > 0) {
			$errCode = 'NP1-3';
			$errMsg = $fnResp ;
			fnLogMessageToDb($errCode.$errMsg);
			return array('errCode' => $errCode, 'errMsg' => $errMsg); }
	}

	//create a new game records
	$areaSize=5; $areaWidth=2;
	$fnResp = fnSetUpGameRecs($gameRef, $playerRef, $areaSize, $areaWidth);
	if (strlen($fnResp)>0) {
		$errCode = 'NP1-4';
		$errMsg = $fnResp;
		fnLogMessageToDb($errCode.$errMsg);
		return array('errCode' => $errCode, 'errMsg' => $errMsg);}

	//return the game ref which means all OK
	return array('gameRef' => $gameRef);
}
//End fnProcessNewP1
function fnSetUpGameRecs($gameRef, $playerRef, $areaSize, $areaWidth) {
	global $conn, $logLevel;
 	fnLogMessageToDb("start fnSetUpGameRecs");

	//begin database transaction
	$errCode = ""; $errMsg = "";
	$conn->beginTransaction();

	//insert new record into the Games table
	$fnResp = fnQryAddGame($gameRef, $playerRef, 0, 0, date("Y-m-d H:i:s"),date("Y-m-d H:i:s"), $areaSize, $areaWidth, 0, 0, 0);
	if (strlen($fnResp)>0) {
		$errCode = "SGR-2";
		$errMsg = "Set up Game Recs - " . $fnResp;
		fnLogMessageToDb($errCode.$errMsg); }

	//insert new records into the Stacks table
    for ($trackIx=0; $trackIx < $areaWidth; $trackIx++) {
		for ($stepIx = 0; $stepIx <= $areaSize + 1; $stepIx++) {
			if (strlen($errCode)>0) { break; }
			$stackNum = '8' . $trackIx . $stepIx;
			if ($stepIx == 0) {
				//set up the player 1 reserve
				$counters = str_repeat('1',$areaSize);
				$fnResp = fnQryAddStack($gameRef, $stackNum, $areaSize, 1, $counters, 0);
				if (strlen($fnResp)>0) {
					$errCode = "SGR-1";
					$errMsg = "Set up Game Recs - " . $fnResp;
					fnLogMessageToDb($errCode.$errMsg); }
			}
			else {
				$fnResp = fnQryAddStack($gameRef, $stackNum, 0, 0, '', 0);
				if (strlen($fnResp)>0) {
					$errCode = "SGR-4";
					$errMsg = "Set up Game Recs - " . $fnResp;
					fnLogMessageToDb($errCode.$errMsg);	}
			}
			if (strlen($errCode)>0) { break; }
		}
	}
	//end database transaction
	if (strlen($errCode)>0) {
		//if an error has occurred rollback any database uopdates
		$conn->rollback(); }
	else {
		$conn->commit(); }
	return $errCode . $errMsg;
}
//End fnSetUpGameRecs
function fnProcessNewP2($tbP2NewGameRef, $tbP2NewPlyrName, $silent, $password = "", $hint = ""){
 	fnLogMessageToDb("start fnProcessNewP2");

	global $tbP1MoveCount, $tbP1MovePos, $tbP2MoveCount, $tbP2MovePos, $logLevel ;

	$playerRef = 0;
	$fnResp =0;
	$errMsg = "";
	$refsRow = null; $playerRow = null;

	//if player name is not blank then see if it already exists
	if (strlen($tbP2NewPlyrName) > 0) {
		// . "new player name: " . $tbP2NewPlyrName .".";
		$playerRow = fnQrySelectPlayer(strtoupper(trim($tbP2NewPlyrName)));
		if (is_array($playerRow)) {
		$playerRef = $playerRow['PlayerRef']; } }

	if ($playerRef == 0) {
		//get a new player ref
		$refsRow = fnReferencesTable("new player ref");
		if  (is_string($refsRow)) {
			$errCode = "NP2-1";
			$errMsg = "New player 2 - error setting up new player references";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		else {
			$playerRef = $refsRow['NextPlayerRef']; } }

	//if player name is not blank and its not already on record then associate input name with new player
	if (strlen($tbP2NewPlyrName) > 0 And is_string($playerRow)) {
		//adding new player
		$fnResp = fnQryAddPlayer($playerRef, strtoupper(trim($tbP2NewPlyrName)),$password,$hint);
		if (strlen($fnResp) > 0) {
			$errCode = "NP2-6";
			$errMsg = "New player 2 - " . $fnResp ;
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
	}

	//display player ref
	$tbP2NewPlyrRef = $playerRef;

	//'set up player 2 reserve in database
	$fnResp = fnSetUpPlayer2($playerRef, $tbP2NewGameRef, $password, $hint);
        if (substr($fnResp,0,1) == 1) {
			$errMsg = substr($fnResp,1); //"New player 2 - invalid game ref";
			$errCode = "NP2-2";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif (substr($fnResp,0,1) == 2) {
			$errMsg = substr($fnResp,1); //"New player 2 - player 2 already exists";
			$errCode = "NP2-4";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif (substr($fnResp,0,1) == 3) {
			$errMsg = substr($fnResp,1); //"New player 2 - database updates failed";
			$errCode = "NP2-7";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }
		elseif (substr($fnResp,0,1) > 0) {
			$errMsg = substr($fnResp,1); //"New player 2 - unknown error";
			$errCode = "NP2-5";
			fnLogMessageToDb($errCode.$errMsg);
			return array("errCode"=>$errCode, "errMsg"=>$errMsg); }

	//clear any player input
	$tbP1MoveCount = ""; $tbP1MovePos = ""; $tbP2MoveCount = ""; $tbP2MovePos = "";

	//return empty errMsg which means all OK
	return $errMsg;
}
//End fnProcessNewP2
function fnSetUpPlayer2($playerRef, $tbP2NewGameRef, $password, $hint) {
 	fnLogMessageToDb("start fnSetUpPlayer2");
	global $conn, $logLevel;

	$areaSize =0;

	//find game record
	$gameRow = fnQrySelectGame($tbP2NewGameRef);
	if (is_string($gameRow)) {
		return 1 . $$gameRow; }

	//the player 2 ref should be zero
	if ($gameRow['P2RecID'] <> 0) {
		return 2 . 'game already has a P2 set';}

	//begin database transaction
	$errCode = ""; $errMsg = "";
	$conn->beginTransaction();

	$fnResp = fnQryUpdateGame($tbP2NewGameRef,$gameRow['P1RecID'],$playerRef,$playerRef,date("Y-m-d H:i:s"),$gameRow['Winner'],$gameRow['TurnCount'] );
	if (strlen($fnResp) > 0) {
		$errCode = "SP2-1";
		$errMsg = $fnResp ;
		fnLogMessageToDb($errCode.$errMsg); }

	$areaSize = $gameRow['AreaSize'];

	//find the GameStep records for this the player 2 reserve
	$stackRows = fnQrySelectStacks($tbP2NewGameRef,0);

	//set height of P2 reserve and save record
	$counters = str_repeat('2', $areaSize);
	for ($trackIx=0; $trackIx < $gameRow['AreaWidth']; $trackIx++) {
		$stackNum = intval('8' . strval($trackIx) . strval($areaSize + 1));
		$fnResp = fnQryUpdateStack($stackNum, $areaSize, 2, $counters, $stackRows[$stackNum]['StackRecID']);
		if (strlen($fnResp) > 0) {
			$errCode = "SP2-2";
			$errMsg = $fnResp ;
			fnLogMessageToDb($errCode.$errMsg); }
	}

	//end database transaction
	if (strlen($errCode)>0) {
		//if an error has occurred rollback any database uopdates
		$conn->rollback();
		return '3' . $errCode . $errMsg; }
	else {
		$conn->commit(); }

	return 0;
}
//End fnSetUpPlayer2
function fnStateOfPlay($gameKey, $turnCount=0) {
 	fnLogMessageToDb("start fnStateOfPlay for ".$gameKey);
	global $tbP1NewGameRef;
	global $logLevel;
	$errMsg = ""; $errCode = ""; $sopData = "";
	$gameRef = ""; $hashKey = "";

	//get game record
	if (strlen($gameKey)<20) {
		fnLogMessageToDb("get game record using game ref");
		$gameRef = $gameKey;
		$gameRow = fnQrySelectGame($gameRef, $turnCount);
		if (is_array($gameRow)==False) {
			$errCode = "SoP-1";
			//$errMsg = "Game " . $gameRef . " and turn count " . $turnCount . " not found in fnQrySelectGame";
			$errMsg = $gameRow;
			fnLogMessageToDb($errCode.$errMsg); }
	}
	else {
		fnLogMessageToDb("get game record using hashKey");
		$hashKey = $gameKey;
		$gameRow = fnQrySelectNmxGame($hashKey);
		if (is_string($gameRow)) {
			$errCode = "SoP-4";
			$errMsg = $gameRow;
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			$gameRow['P1Name'] = "One";
			$gameRow['P2Name'] = "Two";
			if ($gameRow['P1RecID'] == $gameRow['LastPlayerID']) {$gameRow['LPName'] = $gameRow['P2Name'];} else {$gameRow['LPName'] = $gameRow['P1Name'];} }
	}
	if (strlen($errCode)==0) {
		//calculate dimensions
		$sideSpacer = 8;
		$areaSize = $gameRow['AreaSize'];

		//start building output list
		$sopArray = array("type" => "game_status", "GameRef" => $gameRow['GameRef'], "Player1Name" => $gameRow['P1Name']) ;
		$sopArray += array("Player2Name" => $gameRow['P2Name'], "TurnCount" => $gameRow['TurnCount']);
		$sopArray += array("LastPlayer" => $gameRow['LPName']);
		$sopArray += array("AreaSize" => $gameRow['AreaSize'], "AreaWidth" => $gameRow['AreaWidth']);
		if ($gameRow['Winner'] == $gameRow['P1RecID']) {
			//$sopData = $sopData . $gameRow['P1Name'];
			$sopArray += array("Winner" => $gameRow['P1Name']); }
		elseif ($gameRow['Winner'] == $gameRow['P2RecID']){
			//$sopData = $sopData . $gameRow['P2Name'];
			$sopArray += array("Winner" => $gameRow['P2Name']);}
		elseif ($gameRow['Winner'] < 0){
			//$sopData = $sopData . "Void";
			$sopArray += array("Winner" => "Void");}
		else {
			$sopArray += array("Winner" => "");}

		//get pstacks record
		if ($hashKey == "") {
			$stackRows = fnQrySelectStacks($gameRef,$turnCount);
			if (is_array($stackRows)==False) {
				$errCode = "SoP-2";
				$errMsg = $stackRows;
				fnLogMessageToDb($errCode.$errMsg);
			}
		}
		else {
			$stackRows = fnQrySelectNmxStacks($hashKey);
			if (is_string($stackRows)) {
				$errMsg = $stackRows;
				$errCode = "SoP-3";
				fnLogMessageToDb($errCode.$errMsg); }
		}
	}
	if (strlen($errCode)==0) {
		//process playing area
		$arrayCount=0;
		$sopArray["Stacks"] = array();
		for ($trackIx=0; $trackIx < $gameRow['AreaWidth']; $trackIx++){
			for ($stepNum=0; $stepNum <= $areaSize+1; $stepNum++) {
				$stackNum = intval('8' . strval($trackIx) . strval($stepNum));
				$sopArray["Stacks"][$arrayCount] = array("Stack" => $stackNum, "Height" => $stackRows[$stackNum]['StackHeight']);
				if ($stackRows[$stackNum]['OwnerPNum'] ==  0) {
					$sopArray["Stacks"][$arrayCount] += array("Top" => ""); }
				else {
					$sopArray["Stacks"][$arrayCount] += array("Top" => 'P' . $stackRows[$stackNum]['OwnerPNum']); }
				$arrayCount++;
			}
		}
	}

	//if there is an error message then return it otherwise return the state of play data
	if (strlen($errCode) > 0) {
		fnLogMessageToDb($errCode.$errMsg);
		return $errCode.$errMsg;}
	else {
		return $sopArray;}
}
//End fnStateOfPlay
function fnDataChecks($gameKey) {
	fnLogMessageToDb("starting data checks for gameKey ".$gameKey);
	global $logLevel;
	//get game record
	if (strlen($gameKey)<20) {
		$gameRecID=$gameKey; }
	else {
		$gameRecID = 0;
		$hashKey = $gameKey; }

	$errCode = "";
	//get the relevant game, stacks and counters records
	if ($gameRecID>0) {
		//get game record using GameRecID
		fnLogMessageToDb("get game record using GameRecID");
		$gameRow = fnQrySelectGameByID($gameRecID);
		if (is_string($gameRow)) {
			$errCode = "DCH-1";
			$errMsg = $gameRow;
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			fnLogMessageToDb("get counters records using GameRef and TurnCount");
			$countersRows = fnQrySelectAllCounters($gameRow['GameRecID'], $gameRow['TurnCountVer']);
			if (is_string($countersRows)) {
				$errCode = "DCH-3";
				$errMsg = $countersRows;
				fnLogMessageToDb ($errCode.$errMsg); }
			else {
				//get stacks records
				fnLogMessageToDb("get stacks records using GameRef and TurnCount");
				$stackRows = fnQrySelectStacks($gameRow['GameRecID'], $gameRow['TurnCountVer']);
				if (is_string($stackRows)) {
					$errCode = "DCH-2";
					$errMsg = $stackRows;
					fnLogMessageToDb ($errCode.$errMsg); }
			}
		}
	}
	else {
		//get the relevant game, stacks and counters records using hash key
		fnLogMessageToDb("get game record using hashKey");
		$gameRow = fnQrySelectNmxGame($hashKey);
		if (is_string($gameRow)) {
			$errCode = "DCH-4";
			$errMsg = $gameRow;
			fnLogMessageToDb ($errCode.$errMsg); }
		else {
			//get counters records
			fnLogMessageToDb("get counters records using hashKey");
			$countersRows = fnQrySelectAllNmxCounters($hashKey);
			if (is_string($countersRows)) {
				$errCode = "DCH-6";
				$errMsg = $countersRows;
				fnLogMessageToDb ($errCode.$errMsg); }
			else {
				//get stacks records
				fnLogMessageToDb("get stacks records using hashKey");
				$stackRows = fnQrySelectNmxStacks($hashKey);
				if (is_string($stackRows)) {
					$errCode = "DCH-5";
					$errMsg = $stackRows;
					fnLogMessageToDb ($errCode.$errMsg); }
			}
		}
	}
	//check for errors
	if (strlen($errCode)>0) {
		echo $errCode.$errMsg;
		return $errCode.$errMsg; }

	//check game record
	echo "checking game " . $gameRow['GameRecID'] . "v" . $gameRow['TurnCountVer'] . " (key=" . $gameKey . ")" . "<br>";
	//Must be areaSize + 2 stack records with the same gameKey
	if (count($stackRows) != $gameRow['AreaSize'] + 2) {
		$errCode = "DCH-7";
		$errMsg = "Wrong number of stacks " . count($stackRows);
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg);}
	//Must be (areaSize + 2) * 2 counters records with the same gameKey
	if (count($countersRows) != ($gameRow['AreaSize'] + 2)*2) {
		$errCode = "DCH-8";
		$errMsg = "Wrong number of counters " . count($countersRows);
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg);}

	//check stack records
	//The Top for each stack must be zero or it must match one of the player refs in the Game record
	$maxI = count($stackRows);
	for ($i=0; $i<$maxI; $i++) {
		if ($stackRows[$i]['OwnerPNum'] != $gameRow['P1RecID'] and $stackRows[$i]['OwnerPNum'] != $gameRow['P2RecID'] and $stackRows[$i]['OwnerPNum'] != 0) {
			$errCode = "DCH-9";
			$errMsg = "Invalid stack. Top = " . $stackRows[$i]['OwnerPNum'] . " for stack " . $stackRows[$i]['StackRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
		//if height is zero the top must be zero
		if ($stackRows[$i]['StackHeight'] == 0 and $stackRows[$i]['OwnerPNum'] != 0) {
			$errCode = "DCH-G";
			$errMsg = "Stack height is zero but Top is not zero on stack " . $stackRows[$i]['StackRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
		else {
			//If the Top is not zero then there must be a counter for the same stack and the same height
			$found = False;
			if ($stackRows[$i]['OwnerPNum'] != 0) {
				$maxJ = count($countersRows);
				for ($j=0; $j<$maxJ; $j++){
					if ($countersRows[$j]['StackNum'] == $i and $stackRows[$i]['OwnerPNum'] == $countersRows[$j]['PlayerRef'] and $stackRows[$i]['StackHeight'] == $countersRows[$j]['CounterHeight']) {
						$found = True;
						break; }
				}
				if ($found == False) {
					$errCode = "DCH-E";
					$errMsg = "No top counter for stack " . $stackRows[$i]['StackRecID'];
					echo $errCode.$errMsg."<br>";
					fnLogMessageToDb($errCode.$errMsg); }
			}
		}
	}

	//check counters records
	//The counter records for a particular stack must be sequential counterHeight starting at 1
	$lastStackNum = -1;
	$maxI = count($countersRows);
	for ($i=0; $i<$maxI; $i++) {
		//if stack num changes then counterHeight must be 1 otherwise counterHeight must be 1 greater than the previous one
		if ($countersRows[$i]['StackNum'] != $lastStackNum) {
			if ($countersRows[$i]['CounterHeight'] != 1) {
				$errCode = "DCH-A";
				$errMsg = "counter height not equal to 1 for counter " . $countersRows[$i]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg); }
			//The highest counter for a particular stack must match the stackHeight of the corresponding stack record
			if ($i > 0 and $countersRows[$i-1]['CounterHeight'] != $stackRows[$countersRows[$i-1]['StackNum']]['StackHeight']) {
				$errCode = "DCH-C";
				$errMsg = "counter height doesn't match stack for counter " . $countersRows[$i-1]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg); }
			//reset lastStackNum
			$lastStackNum = $countersRows[$i]['StackNum'];
		}
		else {
			if ($countersRows[$i]['CounterHeight'] != $countersRows[$i-1]['CounterHeight'] + 1) {
				$errCode = "DCH-B";
				$errMsg = "counter height not incrementing by 1 for counter " . $countersRows[$i]['CounterRecID'];
				echo $errCode.$errMsg."<br>";
				fnLogMessageToDb($errCode.$errMsg);
			}
		}
		//The Player ref for each counter must match one of the player refs in the Game record
		if ($countersRows[$i]['PlayerRef'] != $gameRow['P1RecID'] and $countersRows[$i]['PlayerRef'] != $gameRow['P2RecID']) {
			$errCode = "DCH-F";
			$errMsg = "invalid player ref on counter " . $countersRows[$i]['CounterRecID'];
			echo $errCode.$errMsg."<br>";
			fnLogMessageToDb($errCode.$errMsg); }
	}
	//The highest counter for a particular stack must match the stackHeight of the corresponding stack record
	$i = count($countersRows);
	if ($i > 0 and $countersRows[$i-1]['CounterHeight'] != $stackRows[$countersRows[$i-1]['StackNum']]['StackHeight']) {
		$errCode = "DCH-D";
		$errMsg = "counter height doesn't match stack for counter " . $countersRows[$i-1]['CounterRecID'];
		echo $errCode.$errMsg."<br>";
		fnLogMessageToDb($errCode.$errMsg); }
	if (strlen($errCode)==0) {
		fnLogMessageToDb("No errors data errors found");
		echo "No errors data errors found";
	}
}
//End fnDataChecks
function fnErrorHandler($errorLevel,$errorMessage,$errorFile="",$errorLine="",$errorContext="") {
 	//fnLogMessageToDb("start fnErrorHandler");
	global $callString, $logLevel;
	$errString = date("Y-m-d H:i:s")." error level=>".$errorLevel."< error message=>".$errorMessage;
	if (strlen($errorFile) != 0) {
		$errString = $errString."< error file=>".$errorFile."< ";}
	if (strlen($errorLine) != 0) {
		$errString = $errString."< error line=".$errorLine."< "; }
	$errString = $errString."\n";

	if (strpos(strtoupper($_SERVER['SCRIPT_NAME']),'BATCH') !== False){
		file_put_contents('./comment_log.txt',$errString,FILE_APPEND);
	}
	else if (strtoupper(substr($_SERVER['DOCUMENT_ROOT'],17,8) == 'ICODE-GO')) {
		file_put_contents($_SERVER['DOCUMENT_ROOT'].'/stacks2/Logs2/comment_log.txt',$errString,FILE_APPEND);
	}
	else if ($_SERVER['SERVER_NAME'] == 'localhost') {
		if (substr($callString,0,4)=="cron") {
			file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$errString,FILE_APPEND); }
		else {
			file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$errString,FILE_APPEND); }
	}
	else if ( substr($_SERVER['DOCUMENT_ROOT'],-6) == 'stacks') {
			file_put_contents($_SERVER["DOCUMENT_ROOT"]."/Logs/comment_log.txt",$errString,FILE_APPEND); }
	else {
		file_put_contents($_SERVER["DOCUMENT_ROOT"]."/stacks/Logs/comment_log.txt",$errString,FILE_APPEND);
	}

}
//End fnErrorHandler
function fnCommentLog($comment) {
	$commentString = date("Y-m-d H:i:s")." comment=>".$comment."< \n";

	if (strpos(strtoupper($_SERVER['SCRIPT_NAME']),'BATCH') !== False){
		file_put_contents('./comment_log.txt',$commentString,FILE_APPEND);
	}
	else if (strtoupper(substr($_SERVER['DOCUMENT_ROOT'],17,8) == 'ICODE-GO')) {
		file_put_contents($_SERVER['DOCUMENT_ROOT'].'/stacks2/Logs2/comment_log.txt',$commentString,FILE_APPEND);
	}
	else if ($_SERVER['SERVER_NAME'] == 'localhost') {
		file_put_contents("C:/xampp/htdocs/stacks2/Logs2/comment_log.txt",$commentString,FILE_APPEND);
	}
	else if (substr($_SERVER['DOCUMENT_ROOT'],-6) == 'stacks') {
		file_put_contents($_SERVER["DOCUMENT_ROOT"]."/Logs/comment_log.txt",$commentString,FILE_APPEND);
	}
	else {
		file_put_contents($_SERVER["DOCUMENT_ROOT"]."/stacks/Logs/comment_log.txt",$commentString,FILE_APPEND);
	}
}
//End fnCommentLog

?>
