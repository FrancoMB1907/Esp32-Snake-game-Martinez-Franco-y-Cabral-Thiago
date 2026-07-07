from django.db import models

class Partida(models.Model):
    puntaje = models.IntegerField()
    jugador = models.CharField(max_length=100)
    fecha = models.DateTimeField(auto_now_add=True)

    class Meta:
        db_table = 'partidas'  # Usa la tabla que ya existe en Supabase
        managed = False        # Django NO crea ni modifica esta tabla
        ordering = ['-puntaje']

    def __str__(self):
        return f"{self.jugador} - {self.puntaje} pts"