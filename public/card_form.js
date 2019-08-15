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

	$("#contents").on("click", "#card-form button.submit", function(evt) {
		console.log("submit button pressed");

		// Get current data from form.
		var form = $("#card-form")[0];
		var data = {};
		for (var i = 0; i < form.length; ++i) {
			var input = form[i];
			if (input.name && input.value != "")
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
