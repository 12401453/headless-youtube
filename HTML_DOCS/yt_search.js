const getResultWidth = () => {
  const viewport_width = window.visualViewport.width;
  const max_result_width = 475;
  return viewport_width > 500 ? max_result_width : viewport_width /** 0.95 */;
};

/* delete me */ let json_results = Object.create(null);
const searchBox = document.getElementById("search_box");
searchBox.focus();
const sendMessage = (event) => {
  if(event.key == "Enter") {
    event.preventDefault();
    let send_data = "message="+encodeURIComponent(searchBox.value.trim()); //this has all been URI-encoded already
    console.log(send_data);
    const httpRequest = (method, url) => {
      const xhttp = new XMLHttpRequest();
      xhttp.open(method, url, true);
      xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
      xhttp.responseType = 'json';
      xhttp.onreadystatechange = () => { 
        if (xhttp.readyState == 4) {
          searchBox.value = "";
          /*const */json_results = xhttp.response["contents"]["twoColumnSearchResultsRenderer"]["primaryContents"]["sectionListRenderer"]["contents"];
          const itemSectionRenderer_no = json_results.length - 2;
          const renderers = json_results[itemSectionRenderer_no]["itemSectionRenderer"]["contents"];

          for(const renderer of renderers) {
            if(Object.keys(renderer)[0] == "videoRenderer") {
              const thumbnails_arr = renderer["videoRenderer"]["thumbnail"]["thumbnails"];
              const thumbnail_src = thumbnails_arr[thumbnails_arr.length - 1]["url"];;
              const runtime = renderer["videoRenderer"]["lengthText"]["simpleText"];
              const deets = [thumbnail_src, runtime];
              addResult(deets);
            }
          }

        }
      }; 
      xhttp.send(send_data);
    };  
    httpRequest("POST", "yt_search.php");  
  }
}
searchBox.addEventListener('keypress', sendMessage);

const addResult = (deets) => {
  const result_width = getResultWidth();
  //const thumb_height = result_width * (101/180);
  //const details_height = thumb_height * (121/404);
  const result_height = result_width * (7823/9090);
  let results_node = document.createRange().createContextualFragment(`<div class="result_block"><div class="thumbnail_square"><img class="thumbnail_img" src="${deets[0]}"><div class="runtime_overlay">${deets[1]}</div></div><div class="details_square"><div class="channel_logo_block"></div><div class="video_details_block"></div></div></div>`);
  //results_node.querySelector(".thumbnail_square").style.height = `${thumb_height}px`;
  //results_node.querySelector(".details_square").style.height = `${details_height}px`;
  results_node.querySelector(".result_block").style.height = `${result_height}px`;
  document.getElementById("results_column").append(results_node);
};
const resizeResults = () => {
  const result_width = getResultWidth();
  const new_result_height = result_width * (7823/9090);
  const result_blocks = document.getElementsByClassName("result_block");
  for(const result_block of result_blocks) {
    result_block.style.height = `${new_result_height}px`;
  }
};
window.addEventListener('resize', resizeResults);