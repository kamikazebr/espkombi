<script lang="ts">
  import Bottle from './lib/Bottle.svelte'
  import {onMount} from 'svelte'


  let btn_shower;
  let shower_status = "0";

  function clickShower() {
      console.log('clickShower shower_status',shower_status);
      btn_shower.textContent = "Carregando...";
      btn_shower.ariaDisabled = true;
      if (shower_status){
        let send_status = shower_status == "0" ? "1" : "0";
        console.log('send_status',send_status);
        setTextShowerButton("Enviando...",false);
        fetch(`/shower?p=${send_status}`).then(res=>res.json()).then(json=>{
          // let text = send_status == "1" ? "Desligar": "Ligar";
          // btn_shower.textContent = text;
        }).catch(reason=>{
          console.log(reason);
          btn_shower.textContent="Erro!";
        }).finally(()=> btn_shower.ariaDisabled = false);
      }
    }


    function setStateShowerButton(toPowerOn=true) {
    setTextShowerButton(toPowerOn?"Ligar":"Desligar",false);
    if (toPowerOn){
      btn_shower.classList.remove("button2");
    }else{
      btn_shower.classList.add("button2");
    }
  }

  function setTextShowerButton(new_text, isDisabled = false) {
    if (new_text !=undefined){
      btn_shower.textContent = new_text;
    }

    btn_shower.ariaDisabled = isDisabled;
  }

  function updateShowerButton(new_status) {
    // console.log('updateShowerButton new_status',new_status);
    shower_status = new_status;
    setTextShowerButton(new_status==1?"Desligar":"Ligar",false);
    setStateShowerButton(new_status==1?false:true);
  }

  function updateLevels(arrStatus) {
    const listLvls = document.querySelectorAll("#lvl")
    for (let i = arrStatus.length - 1; i >= 0; i--) {
      const status = arrStatus[i];
      // console.log("status", status);
      if (status === 1) {
        listLvls[i].classList.add('water-selected')
      } else {
        listLvls[i].classList.remove('water-selected')
      }
    }
  }

  onMount(()=>{
    if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
      console.log("Events Connected");
    }, false);
    source.addEventListener('error', function (e) {
      if ((e.target as EventSource).readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
    }, false);

    source.addEventListener('shower_box', function (e) {
      console.log("shower_box", e.data);
      const obj = JSON.parse(e.data);
      updateLevels(obj["cx1"]);
    }, false);

    source.addEventListener('shower_status', function (e) {
      console.log("shower_status", e.data);
      updateShowerButton(e.data);
    }, false);

    source.addEventListener('controller_status', function (e) {
      try {
        const obj = JSON.parse(e.data);
        console.log("controller_status obj:", obj);
      } catch (error) {
        console.log("error with controller_status:", e.data);
      }
    }, false);

    source.addEventListener('time', function (e) {
      console.log("date time: ", e.data);
    }, false);
  }
    
})

</script>

<main>
  <h1>Casa Kombi Amora</h1>

  <p>DUCHA</p>

  <p>
    <button bind:this={btn_shower} id="btn_shower" class="button" on:click={clickShower}>Ligar</button>
  </p>
  
  <div>
    <span class="sensor-labels">Caixa do Banho</span>

    <Bottle />

  <p>
    <a href="/update"><button class="button">Update OTA</button></a>
    <a href="/edit"><button class="button">Edit Files</button></a>
    <a href="/webserial"><button class="button">WebSerial</button></a>
  </p>

</main>

<style>
      .sensor-labels {
  font-size: 1.5rem;
  vertical-align: middle;
  padding-bottom: 15px;
}
/***
   Rui Santos
   Complete project details at https://RandomNerdTutorials.com
***/

/* html {
  font-family: Arial;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}

h1 {
  color: #0F3376;
  padding: 2vh;
}

p {
  font-size: 1.5rem;
}

.button {
  display: inline-block;
  background-color: #008CBA;
  border: none;
  border-radius: 4px;
  color: white;
  padding: 16px 40px;
  text-decoration: none;
  font-size: 30px;
  margin: 2px;
  cursor: pointer;
}

.button2 {
  background-color: #f44336;
} */

/* .units {
  font-size: 1.2rem;
} */


</style>