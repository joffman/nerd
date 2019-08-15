var main = function() {
	console.log("Starting main...");

	// Load all cards into table.
	$.get("/api/v1/cards", function(data) {
		console.log("Got data...");
		data["cards"].forEach(function(card) {
			$("#cards-table")
				.append('<tr><td class="id-td">' + card.id + "</td><td>" + card.title + "</td></tr>");
		});
		$("#cards-table tr").on("click", function() {
			var id = $(this).find("td.id-td")[0].innerText;
			$.get("/api/v1/cards/" + id, function(data) {
				card_form.set_data(data);
				$("#contents").html(card_form.html());
			});
		});
	});

	// Install click-handler for "new card" button.
	$("#new-card").on("click", function() {
		$("#contents").html(card_form.html());
	});
};

$(document).ready(main);
