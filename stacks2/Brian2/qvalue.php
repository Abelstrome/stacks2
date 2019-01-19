<?php 

require 'C:/xampp/htdocs/stacks2/Brian2/Functions21-5.inc';
fnDbConnect();
$logLevel=3;

$qStore = [];
//Q Store entries comprise [boardMove, qValue] where boardMove is the concatenation of board + moveDir + movePos
//qStoreEntry = array('boardMove' => '', 'qValue' => 0];

fnGetQValue($qStore, $board, $moveDir, $movePos) {
	//get qvalue from qStore
	$boardMove = $board . 'x' . $moveDir . 'x' . $movePos;
	$index = array_search($boardMove, array_column($qStore, 'boardMove')
	$qValue = $qStore[$index]['qvalue'];
}
fnStoreNewQValue($board, $moveDir, $movePos, $qValue) {
	//store new qvalue
	$qStore[] = array('boardMove' => $board . 'x' . $moveDir . 'x' . $movePos, 'qValue' => $qValue);
}

fnGetBestmove($board){
	//get move with the highest qvalue
	//assume board is equiavalent to the gameref
	$movesList = fnGetMoves($gameRow['GameRecID'], $gameRow, $stackRows);
	$count = count($movesList);
	$bestQValue = -9999;
	$bestMoveDir = 0;
	$bestMovePos = 0;
	for ($i = 0; $i < $count; $i++) {
		$qValue = fnGetQValue($qStore, $board, $movesList[1], $movesList[2]);
		if ($qValue > $bestQValue) {
			$bestQValue = $qValue;
			$bestMoveDir = $movesList[1];
			$bestMovePos = $movesList[2];
		}
	}
	return [$bestQValue, $bestMoveDir, $bestMovePos];
}
?>