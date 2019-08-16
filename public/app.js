var main = function() {
	console.log("Starting main...");

	// Insert card_list into DOM and load it with cards from backend.
	$("#contents").html(card_list.html());
	card_list.load_cards();
};

$(document).ready(main);
