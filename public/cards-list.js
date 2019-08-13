var main = function() {
	console.log("Starting main...");
	$.get("/cards/list", function(data) {
		console.log("Got data...");
		data["cards"].forEach(function(card) {
			$("#cards_table")
				.append("<tr><td>" + card.id + "</td><td>" + card.title + "</td></tr>");
		});
	});
};

$(document).ready(main);
