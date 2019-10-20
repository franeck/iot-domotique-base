# Projet IOT Alarme - Base

## Spécifications de l'api :
> Pour communiquer avec les capteurs

POST /event
```json
  {
    "event": {
      "opening": Boolean,
      "eventTime": Long
    }
  }
```

GET /state
```json
  {
    "devices": [
      {
        "id": Int,
        "closed": Boolean
      }
    ]
  }
```

## Description 

**Monitoring**

La base expose un edpoint pour les capteurs pour poster les changements d'état.

La base s'occupe en parallèle de requêter les capteurs pour collecter leur log enentuel et s'assurer qu'ils sont online.

**Badges**

La base dispose d'un lecteur RFID. Grâce aux deux cartes la base mémorise quel proprietaire est à la maison.

**Mode dormant**

Si en moins un des proprietaires est à la maison, les évenements d'ouverture/ferméture sont ignorés.

**Alarme**

Lors que l'alarme est armé et qu'un changement est detecté une requête est envoyé vers le serveur qui arme l'alerte. L'utilisateur a 15 secondes pour désarmer l'alarme avec ça carte. Si la carte est réconnue alors l'ESP envoit un nouveau message au serveur pour dire de désarmer l'alarme. Sinon le serveur déclanche l'alarme.

Si un capteur ne renvoit pas du ping, alors la base arme l'alarme. Si pendant 15 secondes le capteur n'as pas répondu, la base n'allue pas l'armement et l'alarme est déclanché.

**Mise en disposition des informations**

La base expose un endpoint pour connaître l'état des capteurs.

Elle ne diffuse pas d'informations sur les personnes presentes ou pas à la maison ni sur le fait si l'alarme est dormant ou pas.

