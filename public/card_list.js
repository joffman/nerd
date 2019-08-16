var card_list = function() {
	var $html_elem = $(`
			<div>
				<h1>All Cards</h1>
				<table id="cards-table">
					<tr><th>ID</th><th>Title</th></tr>
				</table>
				<button id="new-card">New card</button>
			</div>`);

	// Load all cards into table.
	var load_cards = function() {
		$.get("/api/v1/cards", function(data) {
			console.log("Got data: ", data);
			data.cards.forEach(function(card) {
				$("#cards-table")
					.append('<tr><td class="id-td">' + card.id + "</td><td>" + card.title + "</td></tr>");
			});
			$("#contents").on("click", "#cards-table tr", function() {
				var id = $(this).find("td.id-td")[0].innerText;
				$.get("/api/v1/cards/" + id, function(data) {
					card_form.set_data(data);
					$("#contents").html(card_form.html());
				});
			});
		});
	};

	// Install click-handler for "new card" button.
	$("#contents").on("click", "#new-card", function() {
		$("#contents").html(card_form.html());
	});

	var html = function() {
		return $html_elem.html();
	};

	return {
		load_cards: load_cards,
		html: html
	};
}();
