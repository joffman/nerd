var topics = function() {
	var $html_elem = $(`
			<div>
				<h1>Topcics</h1>
				<table id="topics-table" class="object-table">
					<tr><th>ID</th><th>Name</th><th>Delete</th></tr>
					<tr id="new-topic-row"><td></td>
						<td><input type="text" name="name" placeholder="Name of new topic..."/></td>
						<td><button id="new-topic-button">Add</button></td>
					</tr>
				</table>
			</div>`);

	var insert_topic = function(id, name) {
		$("#new-topic-row").before('<tr data-topic-id="' + id + '">'
				+ '<td class="id-td">' + id + "</td>"
				+ "<td>" + name + "</td>"
				+ '<td><button class="topic-delete">Delete</button></td></tr>');
	};

	var update_topic = function(id, data) {
		$.ajax({
			url: "/api/v1/topics/" + id,
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

	var create_topic = function(data) {
		$.post("/api/v1/topics", JSON.stringify(data), function(resp_data, status, xhr) {
			if (status === "success" && "id" in resp_data) // && resp_data.success
				insert_topic(resp_data.id, data.name);
		});
	};

	var delete_topic = function(id) {
		$.ajax({
			url: "/api/v1/topics/" + id,
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
				$(`#topics-table tr[data-topic-id="${id}"]`).remove();
			}
		});
	};

	// Load all topics into table.
	var load_topics = function() {
		$.get("/api/v1/topics", function(data) {
			console.log("Got data: ", data);
			data.topics.forEach(function(topic) {
				insert_topic(topic.id, topic.name);
			});

			// TODO Edit topic/name when clicking on name.
			//$("#contents").on("click", "#topics-table tr", function() {
			//	if ($(this).find("td").length > 0) {	// ignore clicks on header
			//		var id = $(this).find("td.id-td").text();
			//		$.get("/api/v1/topics/" + id, function(data) {
			//			topic_form.set_data(data);
			//			$("#contents").html(topic_form.html());
			//		});
			//	}
			//});

			// Delete topic when clicking the delete button.
			$("#contents").on("click", "#topics-table button.topic-delete", function() {
				var id = $(this).parent().parent().find("td.id-td").text();

				console.log("deleting topic with id " + id);
				delete_topic(id);

				return false;
			});
		});
	};

	// Create new topic when clicking the Add-button.
	$("#contents").on("click", "#new-topic-button", function() {
		var name = $('#new-topic-row input[name="name"]').val();
		create_topic({"name": name});

		// Empty input field.
		$('#new-topic-row input[name="name"]').val("");
	});

	var html = function() {
		return $html_elem.html();
	};

	return {
		load_topics: load_topics,
		html: html
	};
}();
