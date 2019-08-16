var main = function() {
	console.log("Starting main...");

	// Insert topic-list into DOM and load it with topics from backend.
	$("#contents").html(topics.html());
	topics.load_topics();
};

$(document).ready(main);
