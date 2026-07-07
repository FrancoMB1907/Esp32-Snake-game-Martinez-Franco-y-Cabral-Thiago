from django.shortcuts import render
from .models import Partida

def home(request):
    return render(request, 'leaderboard/home.html')

def leaderboard(request):
    partidas = Partida.objects.all()
    player = request.GET.get('player', '')
    if player:
        partidas = partidas.filter(jugador__icontains=player)
    return render(request, 'leaderboard/leaderboard.html', {
        'scores': partidas,
        'current_player': player,
    })