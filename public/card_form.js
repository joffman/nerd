var card_form = function() {
	var $html_elem = $(`
			<div>
				<h1>Card Form</h1>
				<form id="card-form" class="card-form-grid">
					<input type="hidden" name="id"/>
					<div>Title:</div><input type="text" name="title"/>
					<div>Question:</div><textarea name="question"/>
					<div>Answer:</div><textarea name="answer"/>

					<div class="button-bar"><button class="submit">Save</button><!--More buttons here--></div>
				</form>
			</div>`);

	var update_card = function(id, data) {
		$.ajax({
			url: "/api/v1/cards/" + id,
			type: "PUT",
			data: JSON.stringify(data),
			contentType: "application/json",	// what we send to the server
			dataType: "json",					// what we expect from the server
			error: function(resp_data, status, xhr) {
				alert("Update failed: " + resp_data.error_msg);
			},
			success: function(resp_data, status, xhr) {
				console.log("resp_data: ", resp_data);
				console.log("status: ", status);
				console.log("xhr: ", xhr);
				alert("Update succeeded! ");
				window.location.href = "index.html";
			}
		});
	};

	var create_card = function(data) {
		$.post("/api/v1/cards", JSON.stringify(data), function(resp_data, status, xhr) {
			console.log("resp_data: ", resp_data);
			console.log("status: ", status);
			console.log("xhr: ", xhr);
			window.location.href = "index.html";
		});
	};

	$("#contents").on("click", "#card-form button.submit", function(evt) {
		console.log("save button pressed");

		// Get current data from form.
		var form = $("#card-form")[0];
		var data = {};
		for (var i = 0; i < form.length; ++i) {
			var input = form[i];
			if (input.name && input.value != "")
				data[input.name] = input.value;
		}
		console.log("data: ", data);

		if ("id" in data) {
			var id = data.id;
			delete data.id;
			update_card(id, data);
		} else {
			create_card(data);
		}

		// Prevent default event handling.
		return false;
	});

	var set_data = function(data) {
		$html_elem.find('input[name="id"]').attr("value", data["id"]);
		$html_elem.find('input[name="title"]').attr("value", data["title"]);
		$html_elem.find('textarea[name="question"]').text(data["question"]);
		$html_elem.find('textarea[name="answer"]').text(data["answer"]);
	};

	var html = function() {
		return $html_elem.html();
	};

	return {
		set_data: set_data,
		html: html
	};
}();
