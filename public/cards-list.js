var main = function() {
	console.log("Starting main...");

	// Load all cards into table.
	$.get("/api/v1/cards", function(data) {
		console.log("Got data...");
		data["cards"].forEach(function(card) {
			$("#cards_table")
				.append("<tr><td>" + card.id + "</td><td>" + card.title + "</td></tr>");
		});
	});

	// Install click-handler for "new card" button.
	$("#new_card").on("click", function() {
		$("body").html(`
				<h1>Card Form</h1>
				<form id="card-form">
					Title: <input type="text" name="title"/>
					Question:<br/>
					<textarea name="question"/>
					Answer:<br/>
					<textarea name="anwer"/>

					<input type="submit" value="Submit"/>
				</form>
				`);
		$("#card-form").on("submit", function(evt) {
			console.log("submit button pressed");

			// Get data from object.
			var form = $(this)[0];		// why is [0] necessary???
			var data = {};
			for (var i = 0; i < form.length; ++i) {
				var input = form[i];
				if (input.name)
					data[input.name] = input.value;
			}
			console.log("data: ", data);

			// Send POST request to create card.
			$.post("/api/v1/cards", JSON.stringify(data), function(resp_data, status, xhr) {
				console.log("resp_data: ", resp_data);
				console.log("status: ", status);
				console.log("xhr: ", xhr);
				window.location.href = "index.html";
			});

			// Prevent default event handling.
			return false;
		});
	});
};

$(document).ready(main);
