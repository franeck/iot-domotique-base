# Projet IOT Alarme - Base

## Spécifications de l'api :
> Pour communiquer avec les capteurs

POST /event
```json
  {
    "event": {
      "sensorId": Int,
      "opening": Boolean
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

La base expose un endpoint pour les capteurs pour poster les changements d'état.

La base s'occupe en parallèle de requêter les capteurs via des get-state pour s'assurer qu'ils sont online.

**Badges**

La base dispose d'un lecteur RFID. Grâce aux quatre cartes la base mémorise quel propriétaire est à la maison.

**Mode dormant**

Si au moins un des propriétaires est à la maison, les évènements d'ouverture/fermeture sont ignorés.

**Alarme**

Lorsque l'alarme est armée et qu'un changement est detecté, une requête est envoyée vers le serveur qui arme l'alerte. L'utilisateur a 15 secondes pour désarmer l'alarme avec sa carte. 
Si la carte est reconnue alors l'ESP envoie une nouvelle requête au serveur pour désarmer l'alarme, si la carte n'est pas reconnue ou qu'aucune carte n'est présentée devant le lecteur avant la fin des 15 secondes alors le serveur déclanche l'alarme.

Si un capteur ne renvoie pas du ping, alors la base arme l'alarme. Si pendant 15 secondes le capteur n'a pas répondu, la base n'enclenche pas l'armement et l'alarme est déclanchée.

**Mise en disposition des informations**

La base expose un endpoint pour connaître l'état des capteurs.

Elle ne diffuse pas d'informations sur la présence des personnes à la maison ni sur le fait que l'alarme est en mode dormant ou non.

