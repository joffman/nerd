var card_list = function() {

	//////////////////////////////////////////////////
	// Private variables
	//////////////////////////////////////////////////

	var $html_elem = $(`
			<div>
				<h1 id="card-list-h1">Cards</h1>
				<table id="cards-table" class="object-table">
					<tr><th>ID</th><th>Title</th><th>Delete</th></tr>
				</table>
				<button id="new-card">New card</button>
			</div>`);

	var _topic_id = 0;


	//////////////////////////////////////////////////
	// CRUD operations
	//////////////////////////////////////////////////

	var delete_card = function(id) {
		$.ajax({
			url: "/api/v1/cards/" + id,
			type: "DELETE",
			dataType: "json",					// what we expect from the server
			error: function(resp_data, status, xhr) {
				var msg = "Deletion failed";
				if ("error_msg" in resp_data)
					msg += ": " + resp_data.error_msg;
				alert(msg);
			},
			success: function(resp_data, status, xhr) {
				console.log("resp_data: ", resp_data);
				console.log("status: ", status);
				console.log("xhr: ", xhr);

				// Remove element from DOM.
				$(`#cards-table tr[data-card-id="${id}"]`).remove();
			}
		});
	};

	//////////////////////////////////////////////////
	// Event handler
	//////////////////////////////////////////////////

	// Open empty card_form when clicking the "New Card" button.
	$("#contents").on("click", "#new-card", function() {
		var data = {
			id: "",
			title: "",
			question: "",
			answer: "",
			topic: _topic_id
		};
		card_form.set_data(data);
		$("#contents").html(card_form.html());
	});

	// Edit card when clicking on it.
	$("#contents").on("click", "#cards-table tr", function() {
		if ($(this).find("td").length > 0) {	// ignore clicks on header
			var id = $(this).find("td.id-td").text();
			$.get("/api/v1/cards/" + id, function(data) {
				card_form.set_data(data);
				$("#contents").html(card_form.html());
			});
		}
	});

	// Delete card when clicking the delete button.
	$("#contents").on("click", "#cards-table button.card-delete", function() {
		var id = $(this).parent().parent().find("td.id-td").text();
		delete_card(id);

		return false;
	});

	
	//////////////////////////////////////////////////
	// Public interface
	//////////////////////////////////////////////////

	// Set topic of this card-list.
	var set_topic = function(topic_id, topic_name) {
		if (topic_id < 0) {
			console.log("invalid topic_id: ", topic_id);
			return;
		}

		_topic_id = topic_id;

		$("#card-list-h1").text("Cards of topic " + topic_name);
	};

	// Load all cards into table.
	var load_cards = function() {
		var url = "/api/v1/cards?topic_id=" + _topic_id;
		$.get(url, function(data) {
			data.cards.forEach(function(card) {
				$("#cards-table").append('<tr data-card-id="' + card.id + '">'
						+ '<td class="id-td">' + card.id + "</td>"
						+ "<td>" + card.title + "</td>"
						+ '<td><button class="card-delete">Delete</button></td></tr>');
			});
		});
	};

	var html = function() {
		return $html_elem.html();
	};

	return {
		load_cards: load_cards,
		html: html,
		set_topic: set_topic
	};
}();
