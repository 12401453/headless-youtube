const getResultWidth = () => {
  const viewport_width = window.visualViewport.width;
  const max_result_width = 475;
  return viewport_width > 500 ? max_result_width : viewport_width /** 0.95 */;
};


/* delete me */ let json_results = Object.create(null);
const searchBox = document.getElementById("search_box");
searchBox.focus();
const sendMessage = (event) => {
  if(event.key == "Enter" || event.type == 'click') {
    event.preventDefault();
    searchBox.removeEventListener('keypress', sendMessage);
    searchBox.readOnly = true;
    const search_query = searchBox.value.trim();
    if(search_query == "") return;
    let send_data = "message="+encodeURIComponent(search_query); //this has all been URI-encoded already
    console.log(send_data);
    const httpRequest = (method, url) => {
      const xhttp = new XMLHttpRequest();
      xhttp.open(method, url, true);
      xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
      xhttp.responseType = 'json';
      xhttp.onreadystatechange = () => { 
        if (xhttp.readyState == 4) {
          searchBox.addEventListener('keypress', sendMessage);
          searchBox.readOnly = false;
          searchBox.select();
          document.getElementById("results_column").innerHTML = "";
          /*const */json_results = xhttp.response["contents"]["twoColumnSearchResultsRenderer"]["primaryContents"]["sectionListRenderer"]["contents"];
          const itemSectionRenderer_no = json_results.length - 2;
          const renderers = json_results[itemSectionRenderer_no]["itemSectionRenderer"]["contents"];

          for(const renderer of renderers) {
            if(Object.keys(renderer)[0] == "videoRenderer") {
              const thumbnails_arr = renderer["videoRenderer"]["thumbnail"]["thumbnails"];
              const thumbnail_src = thumbnails_arr[thumbnails_arr.length - 1]["url"];;
              
              let runtime = "";
              let livestream = 0;
              if(Object.hasOwn(renderer["videoRenderer"], "lengthText")) {
                runtime = renderer["videoRenderer"]["lengthText"]["simpleText"];
              }
              else {
                runtime = "LIVE";
                livestream = 1;
              }
              
              const channel_thumbnails_arr = renderer["videoRenderer"]["channelThumbnailSupportedRenderers"]["channelThumbnailWithLinkRenderer"]["thumbnail"]["thumbnails"];
              const channel_thumbnail_src = channel_thumbnails_arr[channel_thumbnails_arr.length -1]["url"];

              const video_title = renderer["videoRenderer"]["title"]["runs"][0]["text"];
              
              const channel_name = renderer["videoRenderer"]["longBylineText"]["runs"][0]["text"];
              let viewcount = "";
              if(Object.hasOwn(renderer["videoRenderer"]["shortViewCountText"], "simpleText")) {
                viewcount = renderer["videoRenderer"]["shortViewCountText"]["simpleText"];
              }
              else {
                renderer["videoRenderer"]["shortViewCountText"]["runs"].forEach(text_chunk => {
                  viewcount += text_chunk["text"];
                })
                viewcount.trim();
              }
              let upload_time = "";
              if(Object.hasOwn(renderer["videoRenderer"], "publishedTimeText")) {
                upload_time = renderer["videoRenderer"]["publishedTimeText"]["simpleText"];
              }
              let video_byline = '<span dir="auto">'+channel_name+'</span><span> • </span><span dir="auto">'+viewcount+'</span>';
              if(upload_time != "") video_byline+='<span> • </span><span dir="auto">'+upload_time+'</span>';
              
      
              const deets = [thumbnail_src, livestream, runtime, channel_thumbnail_src, video_title, video_byline];
              addResult(deets);
            
            }
          }
          document.getElementById("results_column").style.visibility = "visible";

        }
      }; 
      xhttp.send(send_data);
    };
    document.getElementById("results_column").style.visibility = "hidden";
    httpRequest("POST", "yt_search.php");  
  }
}
searchBox.addEventListener('keypress', sendMessage);
document.getElementById("search_button").addEventListener('click', sendMessage);

const addResult = (deets) => {
  const result_width = getResultWidth();
  //const thumb_height = result_width * (101/180);
  //const details_height = thumb_height * (121/404);
  const result_height = result_width * (7823/9090);
  const results_node = document.createRange().createContextualFragment(`<div class="result_block"><div class="thumbnail_square"><img class="thumbnail_img" src="${deets[0]}"><div class="runtime_overlay" data-livestream="${deets[1]}">${deets[2]}</div></div><div class="details_square"><div class="channel_logo_block"><img class="channel_img" src="${deets[3]}"></div><div class="video_details_block"><div class="video_title">${deets[4]}</div><div class="video_byline">${deets[5]}</div></div></div></div>`);
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

/*
const fetchTesting = () => {
  let thumbnail_urls = [];
  const fetchThumbnails = (thumbnail_src) => {
    const thumbnailRequest = new Request(thumbnail_src);

    fetch(thumbnailRequest)
    .then((response) => {
      if(!response.ok) {
        throw new Error(`HTTP error status ${response.status} on fetching thumbnail`);
      }
      return response.blob();
    })
    .then((response) => {
      thumbnail_urls.push(URL.createObjectURL(response));
    })
    .catch(error => {
      console.log(error);
      thumbnail_urls.push(not_found_thumbnail_URL);
    })
  };
  return thumbnail_urls;
}; */